#include "dispatcher.h"

void Dispatcher::registerService(Service* svc){
    services_.insert({svc.name(), svc});
}

void Dispatcher::dispatch(const RpcMessage& req, RpcMessage* resp){
    auto it = services_.find(req.service());

    if(it == services_.end()){
        resp.set_error_code("SERVICE_NOT_FOUND");
    }
    else{
        it->second->callMethod(req.method(), req, resp);
    }
}