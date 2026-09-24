/*
	封装"一个fd + 关心的事件 + 回调函数"，用data.ptr指向它
*/

#pragma once

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

class EventLoop;

class Channel{
private:
	int fd_;		  //文件描述符
	uint32_t events_; //关心的事件
	EventLoop* loop_; //指向所属的事件循环，用来反调updateChannel/removeChannel
	std::function<void()> readCallback_;	//读事件发生时调用
	std::function<void()> writeCallback_;	//写事件发生时调用
	std::function<void()> closeCallback_;	//关闭事件发生时调用
	std::function<void()> errorCallback_;	//错误事件发生时调用
	int index_;
public:
	Channel(EventLoop* loop, int fd);
	~Channel();
	void setReadCallback(std::function<void()> func);
	void setWriteCallback(std::function<void()> func);
	void setCloseCallback(std::function<void()> func);
	void setErrorCallback(std::function<void()> func);

	int getFd() const;
	uint32_t getEvents() const;
	int getIndex() const;
	void setIndex(int value);

	int enableReading(bool flag_ET = true);
	int enableWriting();
	int disableReading();
	int disableWriting();
	int disableAll();
	bool isReading() const;
	bool isWriting() const;
	void handleEvent(uint32_t revents);
	int update();
	void remove();
};
