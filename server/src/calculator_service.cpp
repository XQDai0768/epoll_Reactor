#include "calculator_service.h"

CalculatorService::CalculatorService() : Service("CalculatorService"){
    addMethod("Add", [this](const rpc::RpcMessage& req,rpc::RpcMessage* resp){
        add(req, resp);
    });
}

void CalculatorService::add(const rpc::RpcMessage& req, rpc::RpcMessage* resp){
    /*暂时不实现*/
}