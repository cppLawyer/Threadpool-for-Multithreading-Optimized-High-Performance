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
		 stop.store(false,std::memory_order_release);
		 pool.reserve(this->total_threads);
		 pool.push_back(std::move(std::thread([&]() {

			 do {
				 while(!(this->stop.load(std::memory_order_consume)) && this->task_queue.empty() && this->pool.size() == 1) {
					 std::this_thread::yield();
				 }
				 
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
				 
				 
			 }while (!(this->stop.load(std::memory_order_consume)) || !this->task_queue.empty() || this->pool.size() != 1); })));
	 }
	 inline void push(std::function<void()>&& f1) {
		 task_queue.push(std::move(f1));
	 }
	 inline void stop_pool() {
		 //manually stop the threadpool
		 this->stop.store(true,std::memory_order_release);
		 if (pool[0].joinable()) {
			 pool[0].join();
		 }
	 }
	 inline void reset_and_restart() {
		 assert(!(pool[0].joinable())); //"Call stop_pool() before the reset_and_restart()"
		 assert(!(task_queue.size() > 0));
		 pool.clear();
		 stop.store(false, std::memory_order_release);
		 pool.push_back(std::move(std::thread([&]() {

			 do {
				 while (!(this->stop.load(std::memory_order_consume)) && this->task_queue.empty() && this->pool.size() == 1) {
					 std::this_thread::yield();
				 }
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
				
			 } while (!(this->stop.load(std::memory_order_consume)) || !this->task_queue.empty() || this->pool.size() != 1); })));


	 }
	 ~threadpool() {
		 if (!stop.load(std::memory_order_consume)) {
			 stop.store(true, std::memory_order_relaxed);
			 //automatic stop
		 }
		 if (pool[0].joinable()) {
			 pool[0].join();
		 }

	 }
	
};

#endif


	
