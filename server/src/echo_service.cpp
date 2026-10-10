#include "echo_service.h"
#include "error_code.h"
#include "echo.pb.h"

EchoService::EchoService() : Service("EchoService"){
    methods_["Echo"] = [this](const rpc::RpcMessage& req,rpc::RpcMessage* resp){
        echo(req, resp); 
    };
}

void EchoService::callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) {
    auto it = methods_.find(method);
    if(it != methods_.end()) it->second();
    else resp->set_error_code(kMethodNotFound);
}

void EchoService::echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp) {
    // 1. 从信封的 payload 解析出业务请求（局部变量）
    rpc::EchoRequest request;
    if (!request.ParseFromString(req.payload())) {
        resp->set_error_code(kMethodNotFound);   // 或专门的“参数解析失败”错误码
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