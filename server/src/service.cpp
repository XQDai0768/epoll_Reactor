#include "service.h"

Service::Service(std::string name){
    name_ = name;
}

const std::string& Service::name() const{
    return name_;
}

