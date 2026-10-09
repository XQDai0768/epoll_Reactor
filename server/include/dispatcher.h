#pragma once

#include <unordered_map>
#include "service.h"

class Dispatcher{
private:
    std::unordered_map<std::string, Service*> services_;
public:
    Dispatcher();
    ~Dispatcher() = default;
    void registerService(Service* svc);                     // 塞进 map
    void dispatch(const rpc::RpcMessage& req, rpc::RpcMessage* resp); // 查 service -> 调 callMethod
};