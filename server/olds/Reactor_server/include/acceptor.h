/*
	封装listenfd和accept逻辑
*/
#pragma once

#include <functional>
#include <sys/socket.h>
#include <fcntl.h>
#include <memory>
#include <cstring>//memset
#include <netinet/in.h>//sockaddr_in
#include <arpa/inet.h>//htons
#include <unistd.h>//close
#include <iostream>
#include <cerrno> //errno
#include <cstdio>//perror

class EventLoop;
class Channel;

class Acceptor{
private:
	EventLoop* loop_;//所属的事件循环
	int listenfd_;//监听socket, Acceptor拥有它
	std::unique_ptr<Channel> acceptChannel_;//listenfd对应的Channel，负责盯读事件
	std::function<void(int)> newConnectionCallback_;//新连接回调，参数是新的clientfd
	uint16_t port_;//监听端口
	bool listening_;//是否已监听，防止重复监听
	int setNonBlock(int fd);
public:
	Acceptor(EventLoop* loop, uint16_t port);
	~Acceptor();
	int listen();
	int handleRead();
	void setNewConnectionCallback(std::function<void(int)> func);
};
