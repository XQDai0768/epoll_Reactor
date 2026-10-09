#pragma once 

#include "service.h"

constexpr int kOk = 0;
constexpr int kServiceNotFound = 1;
constexpr int kMethodNotFound = 2;

class EchoService : public Service{
public:
    EchoService();
    //~EchoService() override;
    void callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override;
    void echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp);
};