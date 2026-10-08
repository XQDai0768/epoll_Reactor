#include "service.h"

Service::Service(std::string name){
    name_ = name;
}

std::string Service::name() const{
    return name_;
}

