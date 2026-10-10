#pragma once

#include "service.h"

class CalculatorService : public Service{
public:
    CalculatorService();
    void add(const rpc::RpcMessage& req, rpc::RpcMessage* resp);
};