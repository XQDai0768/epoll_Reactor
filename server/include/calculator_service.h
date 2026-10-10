#pragma once

#include "service.h"

class CalculatorService : public Service{
public:
    CalculatorService();
    void callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override;
    void add(const rpc::RpcMessage& req, rpc::RpcMessage* resp);
};