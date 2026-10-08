#pragma once 

#include "service.h"

class EchoService : public Service{
    EchoService();
    ~EchoService() override;
    void callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override;
    void echo(const RpcMessage& req, RpcMessage* resp);
};