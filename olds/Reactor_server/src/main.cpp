#include "eventloop.h"
#include "acceptor.h"
#include "tcp_connection.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <cstdlib> //atoi

int main(int argc, char* argv[]){
	if(argc <= 1){
		std::cout << "usage: server [port]" << std::endl;
		return -1;
	}

	std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
	std::unique_ptr<Acceptor> acceptor = std::make_unique<Acceptor>(loop.get(), atoi(argv[1]));

	if(acceptor->listen() == -1){
		std::cout << "Error:listen()" << std::endl;
		return -1;
	}

	std::unordered_map<int, std::unique_ptr<TcpConnection>> connections;

	acceptor->setNewConnectionCallback([&](int fd){
		std::unique_ptr<TcpConnection> tcp_ = std::make_unique<TcpConnection>(loop.get(), fd); 

		tcp_->setCloseCallback([&, fd](){
			connections.erase(fd);
		});

		connections.insert({fd, std::move(tcp_)});
	});

	loop->loop();

	return 0;
}
