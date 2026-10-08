#include "echo_service.h"

EchoService::EchoService() : Service("EchoService"){

}

EchoService::~EchoService(){

}

void EchoService::callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) {
    if (method == "Echo") {
        echo(req, resp);
    } else {
        resp->set_error_code(/* METHOD_NOT_FOUND */);
    }
}

void EchoService::echo(const RpcMessage& req, RpcMessage* resp){

}