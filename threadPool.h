#ifndef THREADPOOL_H
#define THREADPOOL_H
/*
- V2
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
#include <cassert>



class threadpool {
	const uint_fast8_t total_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> pool;
	std::queue<std::function<void()>> task_queue;
	std::atomic<bool> stop;
public:
	threadpool() noexcept {
		stop.store(false, std::memory_order_relaxed);
		pool.reserve(this->total_threads);
		pool.push_back(std::move(std::thread([&]() {
			 do {
				while (!(this->stop.load(std::memory_order_acquire)) && this->task_queue.empty() && this->pool.size() == 1)
					std::this_thread::yield();

				if (!this->task_queue.empty() && this->pool.size() != this->total_threads) {
					this->pool.push_back(std::move(std::thread(std::move(this->task_queue.front()))));
					this->task_queue.pop();
				}
				else {
					if (this->pool[1].joinable()) {
						this->pool[1].join();
						this->pool.erase(this->pool.begin() + 1, this->pool.begin() + 2);
					}
				}
			    } while (!(this->stop.load(std::memory_order_relaxed)) || !this->task_queue.empty() || this->pool.size() != 1); 
			})));
	}


	inline void push(std::function<void()>&& f1) noexcept {
		task_queue.push(std::move(f1));
	}

	inline void stop_pool() noexcept {
		//manually stop the threadpool
		this->stop.store(true, std::memory_order_release);
		if (pool[0].joinable())
			pool[0].join();
	}

	inline void reset_and_restart() noexcept {
		assert(!(pool[0].joinable())); //"Call stop_pool() before the reset_and_restart()"
		pool.clear();
		stop.store(false, std::memory_order_relaxed);
		pool.push_back(std::move(std::thread([&]() {
			do {
				while (!(this->stop.load(std::memory_order_acquire)) && this->task_queue.empty() && this->pool.size() == 1)
					std::this_thread::yield(); //Reschedule thread to do other task// best is to avoid this by using stop
				
				if (!this->task_queue.empty() && this->pool.size() != this->total_threads) {
					this->pool.push_back(std::move(std::thread(std::move(this->task_queue.front()))));
					this->task_queue.pop();
				}
				else {
					if (this->pool[1].joinable()) {
						this->pool[1].join();
						this->pool.erase(this->pool.begin() + 1, this->pool.begin() + 2);
					}
				}
				//memory order relaxed because of the acquire above thread have same view...//
			} while (!(this->stop.load(std::memory_order_relaxed)) || !this->task_queue.empty() || this->pool.size() != 1); 
		})));
	}
	~threadpool() noexcept {
		if (!stop.load(std::memory_order_consume))
			stop.store(true, std::memory_order_relaxed); //automatic stop
		if (pool[0].joinable())
			pool[0].detach();	
	}
};

#endif


	
