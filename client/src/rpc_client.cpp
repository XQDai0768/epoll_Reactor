#include "rpc_client.h"
#include "echo_service.h"
#include "calculator_service.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

RpcClient::RpcClient(const std::string ip_addr, uint16_t port){
    next_request_id_ = 0;

    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ == -1){
        perror("socket():");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_addr, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd_);
        return -1;
    }

    if (connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        close(fd_);
        return -1;
    }
}

rpc::RpcMessage RpcClient::call(const std::string& service,
                     const std::string& method,
                     const std::string& payload){
    rpc::RpcMessage msg;
    msg.set_type(rpc::MESSAGE_TYPE_REQUEST);
    msg.set_request_id(next_request_id_++);
    msg.set_error_code(0);
    msg.set_service(service);
    msg.set_method(method);
    msg.set_payload(payload);

    Buffer sendBuf;
    codec_.encode(msg, &sendBuf);

    size_t sent = 0;
    while (sent < sendBuf.readableBytes()) {
        ssize_t n = send(fd_, sendBuf.peek() + sent, sendBuf.readableBytes() - sent, 0);
        if (n <= 0) { perror("send"); close(fd_); return -1; }
        sent += static_cast<size_t>(n);
    }
    std::cout << "已发送请求 (" << sent << " 字节)\n";

    Buffer recvBuf;
    rpc::RpcMessage resp;

    while (true) {
        char tmp[65536];
        ssize_t r = recv(fd_, tmp, sizeof(tmp), 0);
        if (r == 0) { std::cout << "服务端关闭了连接\n"; break; }
        if (r < 0) { perror("recv"); break; }

        recvBuf.append(tmp, static_cast<size_t>(r));

        ParseResult pr = codec_.decode(&recvBuf, &resp);
        if (pr == ParseResult::kOk) {
            if (resp.error_code() != 0) {
                std::cout << "服务端返回错误码: " << resp.error_code() << "\n";
            } 
            
            break;
        } else if (pr == ParseResult::kNeedMore) {
            continue;
        } else {
            std::cout << "解码响应失败: " << static_cast<int>(pr) << "\n";
            break;
        }
    }

    return resp;
}

rpc::EchoResponse RpcClient::Echo(const std::string& text) {
    rpc::EchoRequest req;
    req.set_text(text);
    std::string payload;
    req.SerializeToString(&payload);

    rpc::RpcMessage resp = call("EchoService", "Echo", payload);

    rpc::EchoResponse result;
    result.ParseFromString(resp.payload());
    return result;
}

int RpcClient::Add(int a, int b) {
    rpc::AddRequest req;
    req.set_a(a); req.set_b(b);
    std::string payload;
    req.SerializeToString(&payload);

    rpc::RpcMessage resp = call("CalculatorService", "Add", payload);

    rpc::AddResponse result;
    result.ParseFromString(resp.payload());
    return result.sum();
}