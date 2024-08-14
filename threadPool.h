#ifndef THREADPOOL_H
#define THREADPOOL_H
/*
- V3.0
+ Improvements in Memory ordering and performance
+ Reduced Memory Usage

 -- Github: cppLawyer -- 

 /////////////////////////////////////////////////////////////////////////////////////////

 Note:

 When using this thread pool, 
 it is wise to use it in release mode because debug mode can cause unexpected behavior, 
 resulting in runtime errors due to the high speed of processing.

*/

/*****************************************************************************************/

#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <future>
#include <cassert>

constexpr size_t CACHE_LINE_SIZE = 64;

template<typename T>
class LockFreeQueue {
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next{nullptr};
    };

    alignas(CACHE_LINE_SIZE) std::atomic<Node*> head;
    alignas(CACHE_LINE_SIZE) std::atomic<Node*> tail;
    std::atomic<size_t> size{0};

public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        while (Node* old_head = head.load(std::memory_order_relaxed)) {
            head.store(old_head->next, std::memory_order_relaxed);
            delete old_head;
        }
    }

    void push(const T& value) {
        Node* new_tail = new Node();
        new_tail->data = std::make_shared<T>(value);
        Node* old_tail = tail.load(std::memory_order_relaxed);
        old_tail->next.store(new_tail, std::memory_order_release);
        tail.store(new_tail, std::memory_order_relaxed);
        size.fetch_add(1, std::memory_order_relaxed);
    }

    bool pop(T& result) {
        Node* old_head = head.load(std::memory_order_relaxed);
        Node* new_head = old_head->next.load(std::memory_order_acquire);

        if (!new_head) return false;

        result = std::move(*new_head->data);
        head.store(new_head, std::memory_order_relaxed);
        delete old_head;
        size.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    bool empty() const noexcept {
        return size.load(std::memory_order_relaxed) == 0;
    }
};

class ThreadPool {
    using Task = std::function<void()>;

    std::vector<std::thread> workers;
    std::vector<LockFreeQueue<Task>> task_queues;
    std::atomic<bool> stop{false};
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> index{0};

    void worker_thread(size_t i) {
        while (!stop.load(std::memory_order_acquire)) {
            Task task;
            bool found_task = false;

            for (size_t n = 0; n < task_queues.size(); ++n) {
                if (task_queues[(i + n) % task_queues.size()].pop(task)) {
                    found_task = true;
                    break;
                }
            }

            if (found_task) {
                task();
            } else {
                std::this_thread::yield(); // Avoid busy-waiting
            }
        }
    }

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : task_queues(num_threads) {
        workers.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this, i] { worker_thread(i); });
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        size_t i = index.fetch_add(1, std::memory_order_relaxed) % task_queues.size();
        task_queues[i].push([task]() { (*task)(); });

        return result;
    }

    void shutdown() noexcept {
        stop.store(true, std::memory_order_release);
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ~ThreadPool() noexcept {
        shutdown();
    }
};

#endif // THREADPOOL_H

