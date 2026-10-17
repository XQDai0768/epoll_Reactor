#include "rpc_channel.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include "channel.h"

RpcChannel::RpcChannel(EventLoop *loop, const char* ip_addr, uint16_t port){
    loop_ = loop;
    ip_ = ip_addr;
    port_ = port;
    fd_ = -1;
    next_request_id_ = 0;
}

RpcChannel::~RpcChannel(){
    closeConnection();
}

int RpcChannel::startConnect(){
    if(fd_ != -1){
        close(fd_);
        fd_ = -1;
    } 

    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ == -1){
        perror("socket():");
        return -1;
    }

    if(setNonBlock(fd_) == -1){
        perror("setNonBlock():");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd_);
        fd_ = -1;
        return -1;
    }

    channel_ = std::make_unique<Channel>(loop_, fd_);
    channel_->setReadCallback([this](){
        handleRead();
    });

    ssize_t ret = ::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if(ret == 0){
        if(channel_->enableReading() == -1){
            std::cout << "Error:enableReading()" << std::endl;
            closeConnection();
            return -1;
        }

        if(connectedCallback_) connectedCallback_();
    }
    else if(ret == -1 && errno == EINPROGRESS){
        channel_->setWriteCallback([this](){
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
                perror("getsockopt SO_ERROR");
                closeConnection();
                return;
            }

            if(err == 0){
                if(channel_->disableWriting() == -1){
                    std::cout << "Error:disableWriting()" << std::endl;
                    closeConnection();
                    return;
                }

                if(channel_->enableReading() == -1){
                    std::cout << "Error:enableReading()" << std::endl;
                    closeConnection();
                    return;
                }

                if(connectedCallback_) connectedCallback_();
            }
            else{
                closeConnection();
                return;
            }
	    });

        if(channel_->enableWriting() == -1){
		    std::cout << "Error:enableWriting()" << std::endl;
		    closeConnection();
            return -1;
	    }
    }
    else{
        closeConnection();
        return -1;
    }
    return 0;
}

void RpcChannel::call(const std::string& service, const std::string& method, 
        const std::string& payload, std::function<void(const rpc::RpcMessage&)> cb){
    
    if(fd_ == -1) return;
    uint64_t id = next_request_id_++;

    rpc::RpcMessage req;
    req.set_type(rpc::MESSAGE_TYPE_REQUEST);
    req.set_request_id(id);
    req.set_error_code(0);
    req.set_service(service);
    req.set_method(method);
    req.set_payload(payload);

    pending_[id].callback = cb;

    Buffer sendBuf;
    codec_.encode(req, &sendBuf);

    //发送
    size_t sent = 0;
    while (sent < sendBuf.readableBytes()) {
        ssize_t n = ::send(fd_, sendBuf.peek() + sent, sendBuf.readableBytes() - sent, 0);
        if (n <= 0) {
            perror("send");
            closeConnection();
            return;
        }
        sent += static_cast<size_t>(n);
    }

    std::cout << "已发送请求 (" << sent << " 字节)\n";

}

void RpcChannel::handleRead(){
    auto ret = inputBuffer_.readFd(fd_);
    if(ret == ReturnResult::Closed || ret == ReturnResult::Error){
        closeConnection();
        return;
    }
    
    while(true){
        rpc::RpcMessage resp;
        ParseResult r = codec_.decode(&inputBuffer_, &resp);
        if(r == ParseResult::kOk){
            auto it = pending_.find(resp.request_id());
            if(it != pending_.end()){
                auto cb = std::move(it->second.callback);
                pending_.erase(it);
                if(cb) cb(resp);
                continue;
            }
            else{
                std::cout << "ERROR:Unkown Response" << std::endl;
                continue;
            }
        }
        else if(r == ParseResult::kNeedMore) break;
        else {
            closeConnection();
            break;
        }
    }
}

void RpcChannel::closeConnection(){
    if(fd_ == -1) return;

	if(channel_ != nullptr) channel_->remove();
	close(fd_);
	fd_ = -1;
}

int RpcChannel::setNonBlock(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1) return -1;

	flags |= O_NONBLOCK;

	return fcntl(fd, F_SETFL, flags);
}

void RpcChannel::setConnectedCallback(std::function<void()> func){
    connectedCallback_ = func;
}