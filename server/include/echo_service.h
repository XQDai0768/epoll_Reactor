#pragma once 

#include "service.h"

class EchoService : public Service{
public:
    EchoService();
    //~EchoService() override;
    void callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override;
    void echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp);
};