#include "tcp_server.h"
#include "eventloop.h"

TcpServer::TcpServer(EventLoop* loop, uint16_t port){
	loop_ = loop;
	acceptor_ = std::make_unique<Acceptor>(loop, port);
	thread_ = std::make_unique<ThreadPool>(10);
	running_ = true;
}

TcpServer::~TcpServer(){
	thread_->stop();
}

int TcpServer::start(){

	acceptor_->setNewConnectionCallback([&](int fd){
		if(!running_) return;

		std::shared_ptr<TcpConnection> tcp_ = std::make_shared<TcpConnection>(loop_, fd);
		std::weak_ptr<TcpConnection> ptr_ = tcp_;
		tcp_->setMessageCallback([=](const std::string& data){

			thread_->submit([=](){
				uint32_t len = static_cast<uint32_t>(data.size());
				std::string response;
				response.resize(4 + data.size());

				//写4字节长度
				response[0] = static_cast<char>((len >> 24) & 0xFF);
				response[1] = static_cast<char>((len >> 16) & 0xFF);
				response[2] = static_cast<char>((len >> 8) & 0xFF);
				response[3] = static_cast<char>(len & 0xFF);

				//写消息体
				std::memcpy(&response[4], data.data(), data.size());

				loop_->runInLoop([response, ptr_](){
					auto p = ptr_.lock();
					if(p != nullptr){
						p->send(response, response.length());
					}
				});

			});
		});

		tcp_->setCloseCallback([&, fd](){
			loop_->runInLoop([this, fd](){
				connections_.erase(fd);
			});
		});

		auto ret = connections_.insert({fd, std::move(tcp_)});
		
		if(ret.second == false){
			std::cout << "ERROR:Bad Connection" << std::endl;
		}
		
	});

	if(acceptor_->listen() == -1){
		std::cout << "ERROR:listen()" << std::endl;
		return -1;
	}

	thread_->start();

	return 0;
}

void TcpServer::stop(){
	running_ = false;

	//停止Acceptor
	acceptor_->stop();

	thread_->stop();

	connections_.clear();
}