/*
	封装一条连接的读、写、关闭和它自己的Buffer
*/
#pragma once

#include <memory>
#include <functional>
#include <cstddef>//size_t
#include <cstdint> //uint32_t
#include <iostream>
#include <unistd.h>//write/close
#include <cerrno>//errno
#include "buffer.h"

class Channel;
class EventLoop;

class TcpConnection{
private:
	int clientfd_;
	std::unique_ptr<Channel> channel_;
	Buffer inputBuffer_;
	Buffer outputBuffer_;
	EventLoop* loop_;
	std::function<void()> closeCallback_;
public:
	TcpConnection(EventLoop* loop, int clientfd);
	~TcpConnection();
	int readData();
	void writeData();
	int send(const char* data, size_t len);
	void closeConnection();
	void setCloseCallback(std::function<void()> func);
	int getFd() const;
};
