
#pragma once

#include <memory>
#include <unordered_map>
#include "tcp_connection.h"
#include "acceptor.h"
#include <iostream>
#include "thread_pool.h"
#include "dispatcher.h"
#include "echo_service.h"
#include "calculator_service.h"

class EventLoop;
class ThreadPool;

class TcpServer{
private:
	std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
	std::unique_ptr<Acceptor> acceptor_;
	EventLoop* loop_;
	std::unique_ptr<ThreadPool> thread_;
	bool running_;
	Dispatcher dispatcher_;
	std::unique_ptr<EchoService> echo_service_;
	std::unique_ptr<CalculatorService> calculator_service_;
public:
	TcpServer(EventLoop* loop, uint16_t port);
	~TcpServer();
	int start();
	void stop();
};
