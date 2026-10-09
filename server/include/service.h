#pragma once

#include <string>
#include "service.pb.h"

class Service{
private:
    std::string name_; 
public:
    explicit Service(std::string name);
    virtual ~Service() = default;
    const std::string& name() const;

    // 核心分发入口：纯虚，子类必须实现
    virtual void callMethod(const std::string& method,
                            const rpc::RpcMessage& req,
                            rpc::RpcMessage* resp) = 0;
};