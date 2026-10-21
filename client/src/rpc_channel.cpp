#include "rpc_channel.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include "channel.h"
#include "eventloop.h"
#include "error_code.h"

RpcChannel::RpcChannel(EventLoop *loop, const char* ip_addr, uint16_t port){
    loop_ = loop;
    ip_ = ip_addr;
    port_ = port;
    fd_ = -1;
    next_request_id_ = 0;
    connected_ = false;
    connect_timer_id_ = UINT64_MAX;
}

RpcChannel::~RpcChannel(){
    cleanUp(false);

    channel_.reset();
}

int RpcChannel::startConnect(){
    if(fd_ != -1) closeConnection();

    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ == -1){
        perror("socket():");
        return -1;
    }

    if(setNonBlock(fd_) == -1){
        perror("setNonBlock():");
        close(fd_);
        fd_ = -1;
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
    channel_->setWriteCallback([this](){
        handleWrite();
	});
    channel_->setCloseCallback([this](){
        if(connected_) closeConnection();
        else handleConnectFailed();
    });
    channel_->setErrorCallback([this](){
        if(connected_) closeConnection();
        else handleConnectFailed();
    });

    ssize_t ret = ::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if(ret == 0){
        if(channel_->enableReading() == -1){
            std::cout << "Error:enableReading()" << std::endl;
            closeConnection();
            return -1;
        }

        connected_ = true;

        if(connectedCallback_) connectedCallback_();
    }
    else if(ret == -1 && errno == EINPROGRESS){

        if(channel_->enableWriting() == -1){
		    std::cout << "Error:enableWriting()" << std::endl;
		    closeConnection();
            return -1;
	    }

        connect_timer_id_ = loop_->addTimer(std::chrono::seconds(30), [this](){
            handleConnectFailed();
        });
    }
    else{
        closeConnection();
        return -1;
    }
    return 0;
}

void RpcChannel::call(const std::string& service, const std::string& method, 
        const std::string& payload, std::function<void(const rpc::RpcMessage&)> cb){
    
    uint64_t id = next_request_id_++;

    if(!connected_){
        rpc::RpcMessage resp;
        resp.set_request_id(id);
        resp.set_type(rpc::MESSAGE_TYPE_RESPONSE);
        resp.set_error_code(kNetworkError);

        if(cb) cb(resp);

        return;
    }

    rpc::RpcMessage req;
    req.set_type(rpc::MESSAGE_TYPE_REQUEST);
    req.set_request_id(id);
    req.set_error_code(0);
    req.set_service(service);
    req.set_method(method);
    req.set_payload(payload);

    pending_[id].callback = cb;
    pending_[id].timer_id = loop_->addTimer(std::chrono::seconds(30), [this, id](){
        auto it = pending_.find(id);
        if(it == pending_.end()) return;
        auto call_back = std::move(it->second.callback);
        pending_.erase(id);

        rpc::RpcMessage resp;
        resp.set_request_id(id);
        resp.set_type(rpc::MESSAGE_TYPE_RESPONSE);
        resp.set_error_code(kTimeout);

        if(call_back) call_back(resp);
    });

    codec_.encode(req, &outputBuffer_);

    send();
}

void RpcChannel::handleRead(){
    auto ret = inputBuffer_.readFd(fd_);
    
    while(true){
        rpc::RpcMessage resp;
        ParseResult r = codec_.decode(&inputBuffer_, &resp);
        if(r == ParseResult::kOk){
            auto it = pending_.find(resp.request_id());
            if(it != pending_.end()){
                auto cb = std::move(it->second.callback);
                if(it->second.timer_id != UINT64_MAX) loop_->cancelTimer(it->second.timer_id);
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
            return;
        }
    }

    if(ret == ReturnResult::Closed || ret == ReturnResult::Error) closeConnection();
}

void RpcChannel::handleWrite(){
    if(fd_ == -1) return;

    if(connected_){
        send();
    }
    else{
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
            perror("getsockopt SO_ERROR");
            handleConnectFailed();
            return;
        }

        if(err == 0){
            if(channel_->disableWriting() == -1){
                std::cout << "Error:disableWriting()" << std::endl;
                handleConnectFailed();
                return;
            }

            if(channel_->enableReading() == -1){
                std::cout << "Error:enableReading()" << std::endl;
                handleConnectFailed();
                return;
            }

            if(connect_timer_id_ != UINT64_MAX){
                loop_->cancelTimer(connect_timer_id_);
                connect_timer_id_ = UINT64_MAX;
            }

            connected_ = true;

            if(connectedCallback_) connectedCallback_();
        }
        else{
            handleConnectFailed();
            return;
        }
    }
}

void RpcChannel::send(){

    while (outputBuffer_.readableBytes() > 0) {
        ssize_t n = ::send(fd_, outputBuffer_.peek(), outputBuffer_.readableBytes(), 0);
        if(n > 0) outputBuffer_.retrieve(n);
        else if(n == -1 && errno == EAGAIN){
            if(channel_->enableWriting() == -1){
		        std::cout << "Error:enableWriting()" << std::endl;
		        closeConnection();
                return;
	        }
            break;
        }
        else if(n == -1 && errno == EINTR) continue;
        else{
            perror("send");
            closeConnection();
            return;
        }
    }

    if(outputBuffer_.readableBytes() == 0 && channel_->isWriting() && channel_->disableWriting() == -1){
        std::cout << "Error:disableWriting()" << std::endl;
        closeConnection();
        return;
    }
}

void RpcChannel::handleConnectFailed(){
    if(fd_ == -1 || connected_) return;

    if(connect_timer_id_ != UINT64_MAX){
        loop_->cancelTimer(connect_timer_id_);
        connect_timer_id_ = UINT64_MAX;
    }

    if(channel_ != nullptr && channel_->getIndex() != -1) channel_->remove();

    if(fd_ != -1){
        close(fd_);
        fd_ = -1;
    }

    if(connectFailedCallback_) connectFailedCallback_();
}

void RpcChannel::closeConnection(){
    cleanUp(true);
}

void RpcChannel::cleanUp(bool callback){
    std::unordered_map<uint64_t, std::function<void(const rpc::RpcMessage&)>> cbs;
    for(auto& [key, value] : pending_){
        if(value.timer_id != UINT64_MAX) loop_->cancelTimer(value.timer_id);
        cbs.insert({key, std::move(value.callback)});
    }
    pending_.clear();
    connected_ = false;

    inputBuffer_.retrieveAll();
    outputBuffer_.retrieveAll();

    if(connect_timer_id_ != UINT64_MAX){
        loop_->cancelTimer(connect_timer_id_);
        connect_timer_id_ = UINT64_MAX;
    }

    if(fd_ != -1){
        if(channel_ != nullptr && channel_->getIndex() != -1) channel_->remove();
        close(fd_);
        fd_ = -1;
    }

    if(callback){
        for(auto& [key, value] : cbs){
            rpc::RpcMessage resp;
            resp.set_request_id(key);
            resp.set_type(rpc::MESSAGE_TYPE_RESPONSE);
            resp.set_error_code(kNetworkError);

            if(value) value(resp);
        }
    }
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

void RpcChannel::setConnectFailedCallback(std::function<void()> func){
    connectFailedCallback_ = func;
}