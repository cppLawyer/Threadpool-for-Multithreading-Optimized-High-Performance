#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <functional>
#include <queue>
#include <atomic>

class threadpool {
private:
	const uint_fast16_t total_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> pool;
	std::queue<std::function<void()>> task_queue;
	std::atomic<bool> stop;
 public:
	 threadpool() {
		 stop.store(false,std::memory_order_acq_rel);
		 pool.reserve(this->total_threads);
		 pool.push_back(std::move(std::thread([&]() {

			 do {
				 if (!this->task_queue.empty() && this->pool.size() != this->total_threads) {
					 this->pool.push_back(std::move(std::thread(std::move(this->task_queue.front()))));
					 this->task_queue.pop();
				 }
				 else {
					 if (!this->pool[1].joinable()) {
						 this->pool.erase(this->pool.begin() + 1, this->pool.begin() + 2);
					 }
					 else {
						 this->pool[1].join();
					 }
					 std::this_thread::yield();
				 }

			 } while (!(this->stop.load(std::memory_order_consume)) || this->task_queue.size() != 0 || this->pool.size() != 1); })));
	 }
	 inline void push(std::function<void()>&& f1) {
		 task_queue.push(std::move(f1));
	 }
	 inline void stop_pool() {
		 //manually stop the threadpool
		 this->stop.store(true,std::memory_order_acq_rel);
	 }
	 ~threadpool() {
		 if (!stop.load(std::memory_order_acquire)) {
			 stop.store(true,std::memory_order_acq_rel);
			 //automatic stop
		 }
		 pool[0].join();
	 }
	
};

#endif

	
