#include "acceptor.h"
#include "eventloop.h"
#include "channel.h"

Acceptor::Acceptor(EventLoop* loop, uint16_t port){
	loop_ = loop;
	port_ = port;
	listenfd_ = -1;
	listening_ = false;
}

Acceptor::~Acceptor(){
	if(acceptChannel_ != nullptr){
		loop_->removeChannel(acceptChannel_.get());//智能指针获取
	}

	if(listenfd_ != -1) close(listenfd_);
}

int Acceptor::listen(){
	if(listening_) return -1;

//-----init-----
	//1.socket init
	listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd_ == -1){
		perror("socket:");
		return -1;
	}

	//2.set socket option
	int opt = 1;
	if(setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
		perror("setsockopt");
		close(listenfd_);
		return -1;
	}

	//3.sockaddr_in init
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_);
	addr.sin_addr.s_addr = INADDR_ANY;

	//4.bind
	if(bind(listenfd_, (sockaddr*)&addr, sizeof(addr)) == -1){
		perror("bind");
		close(listenfd_);
		return -1;
	}

	//5.非阻塞listenfd
	if(setNonBlock(listenfd_) == -1){
		std::cout << "ERROR:setNonBlock" << std::endl;
		return -1;
	}

	//6.listen
	if(::listen(listenfd_, 5) == -1){//遮蔽
		perror("listen:");
		return -1;
	}
//------init-------
	acceptChannel_ = std::make_unique<Channel>(loop_, listenfd_);//初始化

	acceptChannel_->setReadCallback([this](){
		handleRead();
	});//回调绑定
	//std::bind(&Acceptor::handleRead, this);

	if(acceptChannel_->enableReading() != 0){
		std::cout << "ERROR:enableReading()" << std::endl;
		return -1;
	}

	listening_ = true;
	return 0;
}

int Acceptor::setNonBlock(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1) return -1;

	flags |= O_NONBLOCK;

	return fcntl(fd, F_SETFL, flags);
}

int Acceptor::handleRead(){
	while(true){
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int clientfd = accept(listenfd_, (struct sockaddr*)&client_addr, &client_len);

		if(clientfd >= 0){
			if(setNonBlock(clientfd) == -1){
				std::cout << "ERROR:setNonBlock" << std::endl;
				return -1;
			}
			std::cout << "Connection:" << clientfd << std::endl;
			if(newConnectionCallback_) newConnectionCallback_(clientfd);
		}
		else if(clientfd == -1 && errno == EAGAIN){
			break;
		}
		else if(clientfd == -1 && errno == EINTR){
			continue;
		}
		else{
			perror("accept:");
			return -1;
		}
	}
	return 0;
}

void Acceptor::setNewConnectionCallback(std::function<void(int)> func){
	newConnectionCallback_ = func;
}
