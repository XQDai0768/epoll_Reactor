#include "eventloop.h"
#include "tcp_server.h"
#include <cstdlib> //atoi
#include <memory>
#include <iostream>
#include <csignal>
#include <sys/eventfd.h>
#include "channel.h"

int efd;

void handleExit(int num){
	uint64_t one = 1;
	write(efd, &one, sizeof(one));
}

int main(int argc, char* argv[]){
	if(argc <= 1){
		std::cout << "usage: server [port]" << std::endl;
		return -1;
	}

	//忽略SIGPIPE信号
	std::signal(SIGPIPE, SIG_IGN);

	

	std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
	std::unique_ptr<TcpServer> tcp_server = std::make_unique<TcpServer>(loop.get(), atoi(argv[1]));

	efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

	if(efd == -1){
		perror("eventfd:");
		return -1;
	}

	std::unique_ptr<Channel> ch_ = std::make_unique<Channel>(loop.get(), efd);

	EventLoop* tmp_ptr = loop.get();
	ch_->setReadCallback([tmp_ptr](){
		uint64_t val;
		ssize_t n = read(efd, &val, sizeof(val));
		if(n == -1){
			perror("eventfd read:");
		}

		tmp_ptr->quit();
	});

	if(ch_->enableReading() == -1){
		std::cout << "Error:enableReading()" << std::endl;
		return -1;
	}

	std::signal(SIGINT, handleExit);
	std::signal(SIGTERM, handleExit);

	if(tcp_server->start() == -1){
		std::cout << "ERROR:start()" << std::endl;
		return -1;
	}

	loop->loop();

	tcp_server->stop();

	ch_->remove();
	close(efd);
	efd = -1;

	return 0;
}
