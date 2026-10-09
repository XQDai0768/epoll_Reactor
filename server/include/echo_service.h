#pragma once 

#include "service.h"
// #include <map>
// #include <string>

// using Handler = std::function<void(const rpc::RpcMessage& req, rpc::RpcMessage* resp)>;

class EchoService : public Service{
// private:
//     map<std::string, Handler> handlers_;
public:
    EchoService();
    ~EchoService() override;
    void callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override;
    void echo(const RpcMessage& req, RpcMessage* resp);
};