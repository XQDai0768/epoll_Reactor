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
		tcp_->setMessageCallback([=](const rpc::RpcMessage& request){

			thread_->submit([=](){
				rpc::RpcMessage response;
				response.set_type(rpc::MESSAGE_TYPE_RESPONSE);
				response.set_request_id(request.request_id());
				response.set_payload(request.payload());
				response.set_error_code(0);
				response.set_service(request.service());
				response.set_method(request.method());

				loop_->runInLoop([response, ptr_](){
					auto p = ptr_.lock();
					if(p != nullptr){
						p->send(response);
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