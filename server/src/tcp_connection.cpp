#include "tcp_connection.h"
#include "buffer.h"
#include "channel.h"
#include "eventloop.h"

TcpConnection::TcpConnection(EventLoop* loop, int clientfd){
	clientfd_ = clientfd;
	loop_ = loop;
	channel_ = std::make_unique<Channel>(loop_, clientfd_);
	timer_id_ = UINT64_MAX;

	channel_->setReadCallback([this](){
		readData();
	});

	channel_->setWriteCallback([this](){
		writeData();
	});

	channel_->setCloseCallback([this](){
		closeConnection();
	});

	if(channel_->enableReading() == -1){
		std::cout << "Error:enableReading()" << std::endl;
		closeConnection();
	}

	timer_id_ = loop_->addTimer(std::chrono::seconds(30), [this](){
		std::cout << "Connection overtime" << std::endl;
		closeConnection();
	});
}

TcpConnection::~TcpConnection(){
	if(clientfd_ != -1){
		channel_->remove();
		close(clientfd_);
		clientfd_ = -1;
	}

	if(timer_id_ != UINT64_MAX){
		loop_->cancelTimer(timer_id_);
		timer_id_ = UINT64_MAX;
	}
}

int TcpConnection::readData(){
	ReturnResult res = inputBuffer_.readFd(clientfd_);
	
	if(res == ReturnResult::Closed || res == ReturnResult::Error){
		closeConnection();
		return -1;
	}
	else{
		while(true){
			rpc::RpcMessage msg;
    		ParseResult r = codec_.decode(&inputBuffer_, &msg);
			if (r == ParseResult::kOk) {
        		if(timer_id_ != UINT64_MAX) loop_->cancelTimer(timer_id_);
				timer_id_ = loop_->addTimer(std::chrono::seconds(30), [this](){
					std::cout << "Connection Timeout" << std::endl;
					closeConnection();
				});

        		if (messageCallback_) messageCallback_(msg);
        		continue;             // 继续解，处理粘包
    		} 
			else if (r == ParseResult::kNeedMore) break;     // 数据不够，等下次读
			else {
        		closeConnection();    // kInvalidLength / kParseError
        		return -1;
    		}
		}
	}
	return 0;
}

void TcpConnection::writeData(){
	if(outputBuffer_.readableBytes() == 0) return;

	//回显
	while(true){
		ssize_t written = write(clientfd_, outputBuffer_.peek(), outputBuffer_.readableBytes());

		if(written > 0){
			outputBuffer_.retrieve(written);
			continue;
		}
		else if(outputBuffer_.readableBytes() == 0) channel_->disableWriting();
		else if(written == -1 && errno == EAGAIN) channel_->enableWriting();
		else if(written == -1 && errno == EINTR) continue;
		else if(written == 0) channel_->enableWriting();
		else closeConnection();
		break;
	}
}

int TcpConnection::send(const rpc::RpcMessage& msg){
	if(clientfd_ == -1) return -1;

	codec_.encode(msg, &outputBuffer_);
	writeData();

	return 0;
}

void TcpConnection::closeConnection(){
	if(clientfd_ == -1) return;

	channel_->remove();
	close(clientfd_);
	clientfd_ = -1;

	if(timer_id_ != UINT64_MAX){
		loop_->cancelTimer(timer_id_);
		timer_id_ = UINT64_MAX;
	} 

	if(closeCallback_) closeCallback_();
}

void TcpConnection::setCloseCallback(std::function<void()> func){
	closeCallback_ = func;
}

void TcpConnection::setMessageCallback(std::function<void(const rpc::RpcMessage&)> func){
	messageCallback_ = func;
}

int TcpConnection::getFd() const{
	return clientfd_;
}
