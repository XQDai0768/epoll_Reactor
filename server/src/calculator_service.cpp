#include "calculator_service.h"
#include "error_code.h"
#include "calculator.pb.h"

CalculatorService::CalculatorService() : Service("CalculatorService"){
    addMethod("Add", [this](const rpc::RpcMessage& req,rpc::RpcMessage* resp){
        add(req, resp);
    });
}

void CalculatorService::add(const rpc::RpcMessage& req, rpc::RpcMessage* resp) {
    // 1. 从信封 payload 解析出 AddRequest
    rpc::AddRequest request;
    if (!request.ParseFromString(req.payload())) {
        resp->set_error_code(kBadRequest);
        return;
    }

    // 2. 计算业务结果
    rpc::AddResponse response;
    response.set_sum(request.a() + request.b());

    // 3. 序列化塞回 payload
    response.SerializeToString(resp->mutable_payload());
}