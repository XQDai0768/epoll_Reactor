#include "thread_pool.h"

ThreadPool::ThreadPool(size_t num){
	threadNums_ = num;
	stopping_ = false;
}

ThreadPool::~ThreadPool(){
	stop();
}

bool ThreadPool::submit(std::function<void()> task){
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if(stopping_) return false;

		tasks_.push(std::move(task));//移交所有权
	}//退出，解锁

	cv_.notify_one();
	return true;
}

void ThreadPool::start(){
	if(!threads_.empty()) return;

	threads_.reserve(threadNums_);//初始化容量

	for(size_t i = 0; i < threadNums_; ++i){
		threads_.emplace_back([this](){
			workerLoop();
		});
	}
}

void ThreadPool::stop(){
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stopping_ = true;
	}

	cv_.notify_all();

	for(auto& thread : threads_){
		if(thread.joinable()) thread.join();
	}
}

void ThreadPool::workerLoop(){
	while(true){
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(mutex_);

			cv_.wait(lock, [this](){
				return stopping_ || !tasks_.empty();
			});

			if(stopping_ && tasks_.empty()) return;

			task = std::move(tasks_.front());
			tasks_.pop();
		}
		
		try{
			task();
		}catch(const std::exception& e){
			std::cout << "捕获到异常" << e.what() << std::endl;
		}catch(...){
			std::cout << "未知异常" << std::endl;
		}
	}
}
