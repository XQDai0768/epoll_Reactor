#pragma once

#include <memory>
#include <functional>
#include <string>
#include <cstdint>
#include "buffer.h"
#include "rpc_codec.h"
#include <unordered_map>
#include <cstdint>

class EventLoop;
class Channel;

struct Pending{
    std::function<void(const rpc::RpcMessage&)> callback;
    std::uint64_t timer_id = UINT64_MAX;
};

class RpcChannel{
private:
    EventLoop *loop_;
    std::unique_ptr<Channel> channel_;
    int fd_;
    std::string ip_;
    uint16_t port_;
    std::function<void()> connectedCallback_;
    uint64_t next_request_id_;
    std::unordered_map<uint64_t, Pending> pending_;
    Buffer inputBuffer_;
    RpcCodec codec_;
    bool connected_;

    int setNonBlock(int fd);
    void handleRead();
    void cleanUp(bool callback);
public:
    RpcChannel(EventLoop* loop, const char* ip_addr, uint16_t port);
    ~RpcChannel();
    int startConnect();
    void closeConnection();
    void setConnectedCallback(std::function<void()> func);
    void call(const std::string& service, const std::string& method, 
        const std::string& payload, std::function<void(const rpc::RpcMessage&)> cb);
};