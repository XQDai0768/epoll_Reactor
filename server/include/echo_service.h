#pragma once 

#include "service.h"

class EchoService : public Service{
public:
    EchoService();
    void echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp);
};