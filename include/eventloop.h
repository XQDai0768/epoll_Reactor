/*
	封装epoll的创建/注册/wait/分发，是事件循环本体
*/
#pragma once

#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <functional>
#include <mutex>
#include <sys/eventfd.h>
#include <stdint.h>//uint64_t
#include <atomic>
#include "timer_queue.h"

class Channel;

class EventLoop{
private:
	int epollfd_;//epoll文件描述符
	std::vector<epoll_event> events_;//epoll_wait返回的事件数组
	std::vector<std::function<void()>> pendingTasks_;//待执行关闭任务列表
	std::atomic<bool> running_;//循环是否继续，quit()时置false
	int eventfd_;
	std::mutex mtx_;
	std::unique_ptr<TimerQueue> timer_;
public:
	EventLoop();
	~EventLoop();
	void loop();
	bool quit();
	int updateChannel(Channel* ch);
	int removeChannel(Channel* ch);
	void runInLoop(std::function<void()> task);
	void addTimer(Duration delay, std::function<void()> func);
    void addTimer(Duration delay, Duration interval, std::function<void()> func);
    void cancelTimer(std::uint64_t id);
};
