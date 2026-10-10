#include "service.h"

Service::Service(std::string name){
    name_ = name;
}

const std::string& Service::name() const{
    return name_;
}

void Service::callMethod(const std::string& method,
                const rpc::RpcMessage& req,
                rpc::RpcMessage* resp) {
    auto it = methods_.find(method);
    if(it != methods_.end()) it->second(req, resp);
    else resp->set_error_code(kMethodNotFound);
}

void Service::addMethod(const std::string& method, Handler handler){
    methods_[method] = handler;
}