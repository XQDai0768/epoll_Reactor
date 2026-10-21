#include "rpc_channel.h"
#include "eventloop.h"
#include <memory>
#include <cstdlib>
#include <iostream>
#include "echo.pb.h"
#include "error_code.h"
#include <csignal>

int main(int argc, char* argv[]){
    if (argc < 3) {
        std::cout << "usage: client <ip> <port>\n";
        return -1;
    }

    //忽略SIGPIPE信号
	std::signal(SIGPIPE, SIG_IGN);

    const char* ip = argv[1];
    uint16_t port = std::atoi(argv[2]);

    std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
    std::unique_ptr<RpcChannel> rpc_channel = std::make_unique<RpcChannel>(loop.get(), ip, port);
    int received = 0;

    rpc_channel->setConnectedCallback([&loop, &received, &rpc_channel](){
        std::cout << "connected" << std::endl;

        rpc::EchoRequest req;
        std::string payload;

        req.set_text("a");
        req.SerializeToString(&payload);
        rpc_channel->call("EchoService", "Echo", payload, [&received, &loop](const rpc::RpcMessage& resp){
            rpc::EchoResponse r;
            if(resp.error_code() == kOk){
                r.ParseFromString(resp.payload());
                std::cout << "resp id=" << resp.request_id() << " text=" << r.text() << "\n";
            }
            else{
                std::cout << resp.error_code() << "\n";
            }

            if(++received == 3) loop->quit();
        });

        req.set_text("b");
        req.SerializeToString(&payload);
        rpc_channel->call("EchoService", "Echo", payload, [&received, &loop](const rpc::RpcMessage& resp){
            rpc::EchoResponse r;
            if(resp.error_code() == kOk){
                r.ParseFromString(resp.payload());
                std::cout << "resp id=" << resp.request_id() << " text=" << r.text() << "\n";
            }
            else{
                std::cout << resp.error_code() << "\n";
            }
            if(++received == 3) loop->quit();
        });

        req.set_text("c");
        req.SerializeToString(&payload);
        rpc_channel->call("EchoService", "Echo", payload, [&received, &loop](const rpc::RpcMessage& resp){
            rpc::EchoResponse r;
            if(resp.error_code() == kOk){
                r.ParseFromString(resp.payload());
                std::cout << "resp id=" << resp.request_id() << " text=" << r.text() << "\n";
            }
            else{
                std::cout << resp.error_code() << "\n";
            }
            
            if(++received == 3) loop->quit();
        });

    });

    rpc_channel->setConnectFailedCallback([&loop]{
        std::cout << "ERROR:connect failed \n";
        loop->quit();
    });

    if(rpc_channel->startConnect() == -1) return -1;

    loop->loop();

    return 0;
}