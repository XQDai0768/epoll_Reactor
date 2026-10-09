#include "echo_service.h"

EchoService::EchoService() : Service("EchoService"){

}

void EchoService::callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) {
    if (method == "Echo") {
        echo(req, resp);
    } 
    else {
        resp->set_error_code(kMethodNotFound);
    }
}

void EchoService::echo(const rpc::RpcMessage& req, rpc::RpcMessage* resp){
    resp->set_payload(req.payload());
}