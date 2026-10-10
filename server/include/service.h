#pragma once

#include <string>
#include "service.pb.h"

using Handler = std::function<void(const rpc::RpcMessage& req, rpc::RpcMessage* resp)>;

class Service{
private:
    std::string name_;
    std::unordered_map<std::string, Handler> methods_;
public:
    explicit Service(std::string name);
    virtual ~Service() = default;
    const std::string& name() const;

    // 核心分发入口：纯虚，子类必须实现
    virtual void callMethod(const std::string& method,
                            const rpc::RpcMessage& req,
                            rpc::RpcMessage* resp) = 0;
};