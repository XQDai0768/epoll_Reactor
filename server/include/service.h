#pragma once

#include <string>
#include "service.pb.h"
#include <unordered_map>
#include <functional>
#include "error_code.h"

using Handler = std::function<void(const rpc::RpcMessage& req, rpc::RpcMessage* resp)>;

class Service{
private:
    std::string name_;
    std::unordered_map<std::string, Handler> methods_;
public:
    explicit Service(std::string name);
    virtual ~Service() = default;
    const std::string& name() const;

    // 核心分发入口
    void callMethod(const std::string& method,
                            const rpc::RpcMessage& req,
                            rpc::RpcMessage* resp);
protected:
    void addMethod(const std::string& method, Handler handler);   // 子类构造时注册
};