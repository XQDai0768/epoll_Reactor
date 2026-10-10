#include "echo_service.h"
#include "error_code.h"
#include "echo.pb.h"

EchoService::EchoService() : Service("EchoService"){
    addMethod("Echo", [this](const rpc::RpcMessage& req,rpc::RpcMessage* resp){
        echo(req, resp);
    });
}

void EchoService::echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp) {
    // 1. 从信封的 payload 解析出业务请求（局部变量）
    rpc::EchoRequest request;
    if (!request.ParseFromString(req.payload())) {
        resp->set_error_code(kBadRequest);   
        return;
    }

    // 2. 处理业务，构造业务响应（局部变量）
    rpc::EchoResponse response;
    response.set_text(request.text());

    // 3. 序列化业务响应，塞回信封的 payload
    std::string payload;
    response.SerializeToString(&payload);
    resp->set_payload(payload);
}