#pragma once

#include "rpc_codec.h"
#include <cstdint>
#include <string>

class RpcClient{
private:
    int fd_;                       // socket
    RpcCodec codec_;               // 复用编解码
    uint64_t next_request_id_;     // request_id 自增

public:
    RpcClient(const std::string ip_addr, uint16_t port);
    ~RpcClient() = defalut;
    rpc::RpcMessage call(const std::string& service,
                     const std::string& method,
                     const std::string& payload);
    rpc::EchoResponse Echo(const std::string& text);
    int Add(int a, int b);
};