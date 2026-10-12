#include <cstdlib>
#include <iostream>
#include "rpc_client.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "usage: client <ip> <port>\n";
        return -1;
    }

    const char* ip = argv[1];
    uint16_t port = std::atoi(argv[2]);

    RpcClient client(ip, port);

    auto echo = client.Echo("hello");
    std::cout << echo.text() << "\n";

    std::cout << "1+2=" << client.Add(1, 2) << "\n";
    return 0;
}
