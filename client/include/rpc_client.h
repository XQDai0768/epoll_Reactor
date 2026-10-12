#pragma once

#include "rpc_codec.h"
#include <cstdint>
#include <string>
#include "echo.pb.h"

class RpcClient{
private:
    int fd_;                       // socket
    RpcCodec codec_;               // 复用编解码
    uint64_t next_request_id_;     // request_id 自增
    std::string ip_;
    uint16_t port_;

    rpc::RpcMessage call(const std::string& service,
                     const std::string& method,
                     const std::string& payload);
    bool connect();
    bool send(Buffer& sendBuf);
    bool recv(rpc::RpcMessage& resp);

public:
    RpcClient(const char* ip_addr, uint16_t port);
    ~RpcClient();
    rpc::EchoResponse Echo(const std::string& text);
    int Add(int a, int b);
};