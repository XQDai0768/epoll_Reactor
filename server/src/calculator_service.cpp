#include "calculator_service.h"

CalculatorService::CalculatorService() : Service("CalculatorService"){
    methods_["Calculator"] = [this](const rpc::RpcMessage& req,rpc::RpcMessage* resp){
        add(req, resp);
    };
}

void CalculatorService::callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) override{
    auto it = methods_.find(method);
    if(it != methods_.end()) it->second();
    else resp->set_error_code(kMethodNotFound);
}

void CalculatorService::add(const rpc::RpcMessage& req, rpc::RpcMessage* resp){
    /*暂时不实现*/
}