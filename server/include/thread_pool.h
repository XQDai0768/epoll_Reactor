
#pragma once

#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <cstddef>
#include <iostream>
#include <stdexcept>

class ThreadPool{
private:
	size_t threadNums_;//线程数量
	std::queue<std::function<void()>> tasks_;//任务队列
	std::mutex mutex_;//互斥锁
	std::condition_variable cv_;//条件变量
	std::vector<std::thread> threads_;//工作线程
	bool stopping_;//停止标志

	void workerLoop();
public:
	ThreadPool(size_t num);
	~ThreadPool();
	void start();
	void stop();
	bool submit(std::function<void()> task);

	ThreadPool(const ThreadPool&) = delete;//禁用
	ThreadPool& operator=(const ThreadPool&) = delete;//禁用
};
