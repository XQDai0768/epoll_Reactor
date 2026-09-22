#include "tcp_connection.h"
#include "buffer.h"
#include "channel.h"

TcpConnection::TcpConnection(EventLoop* loop, int clientfd){
	clientfd_ = clientfd;
	loop_ = loop;
	channel_ = std::make_unique<Channel>(loop_, clientfd_);

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

}

TcpConnection::~TcpConnection(){
	if(clientfd_ != -1){
		channel_->remove();
		close(clientfd_);
		clientfd_ = -1;
	}
}

int TcpConnection::readData(){
	ReturnResult res = inputBuffer_.readFd(clientfd_);
	
	if(res == ReturnResult::Closed || res == ReturnResult::Error){
		closeConnection();
		return -1;
	}
	else{
		while(inputBuffer_.readableBytes() >= 4){
		    uint32_t msg_len;
			memcpy(&msg_len, inputBuffer_.peek(), 4);
			msg_len = ntohl(msg_len);

			if(msg_len > 65535){
				inputBuffer_.retrieveAll();
				closeConnection();
				return -1;
			}

			size_t total_need = 4 + msg_len;
			if(inputBuffer_.readableBytes() < total_need) break;

			//读取消息
			std::string request(inputBuffer_.peek() + 4, msg_len);

			std::cout << "读取长度为" << msg_len << "的消息:" << request << std::endl;

			//处理消息
			if(messageCallback_) messageCallback_(request);
			else{
				inputBuffer_.retrieve(total_need);
				return -1;
			}

			inputBuffer_.retrieve(total_need);
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

int TcpConnection::send(const std::string& data, size_t len){
	if(clientfd_ == -1) return -1;

	if(outputBuffer_.append(data, len) == -1){
		std::cout << "Error:append()" << std::endl;
		return -1;
	}

	writeData();

	return 0;
}

void TcpConnection::closeConnection(){
	if(clientfd_ == -1) return;

	channel_->remove();
	close(clientfd_);
	clientfd_ = -1;

	if(closeCallback_) closeCallback_();
}

void TcpConnection::setCloseCallback(std::function<void()> func){
	closeCallback_ = func;
}

void TcpConnection::setMessageCallback(std::function<void(const std::string&)> func){
	messageCallback_ = func;
}

int TcpConnection::getFd() const{
	return clientfd_;
}
