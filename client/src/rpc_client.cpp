#include "rpc_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "buffer.h"
#include "calculator.pb.h"
#include "error_code.h"
#include <sys/time.h>
#include <cstring>
#include <iostream>

RpcClient::RpcClient(const char* ip_addr, uint16_t port){
    next_request_id_ = 0;
    ip_ = ip_addr;
    port_ = port;

    if(!connect()) return;
}

RpcClient::~RpcClient(){
    if(fd_ != -1){
        close(fd_);
        fd_ = -1;
    }
}

bool RpcClient::connect(){
    if(fd_ != -1){
        close(fd_);
        fd_ = -1;
    } 

    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ == -1){
        perror("socket():");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 3; //秒
    tv.tv_usec = 0; //微秒

    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

bool RpcClient::send(Buffer& sendBuf){
    size_t sent = 0;
    while (sent < sendBuf.readableBytes()) {
        ssize_t n = ::send(fd_, sendBuf.peek() + sent, sendBuf.readableBytes() - sent, 0);
        if (n <= 0) {
            perror("send");
            close(fd_);
            fd_ = -1;
            return false;
        }
        sent += static_cast<size_t>(n);
    }

    std::cout << "已发送请求 (" << sent << " 字节)\n";
    return true;
}

bool RpcClient::recv(rpc::RpcMessage& resp){
    Buffer recvBuf;

    while (true) {
        char tmp[65536];
        ssize_t r = ::recv(fd_, tmp, sizeof(tmp), 0);
        if (r == 0) {
            std::cout << "服务端关闭了连接\n";
            resp.set_error_code(kNetworkError);
            close(fd_);
            fd_ = -1;
            return false;
        }
        else if(r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            std::cout << "超时\n";
            resp.set_error_code(kTimeout);
            close(fd_);
            fd_ = -1;
            return false;
        }
        else if (r < 0) {
            perror("recv");
            resp.set_error_code(kNetworkError);
            close(fd_);
            fd_ = -1;
            return false;
        }

        recvBuf.append(tmp, static_cast<size_t>(r));

        ParseResult pr = codec_.decode(&recvBuf, &resp);
        if (pr == ParseResult::kOk) {
            if (resp.error_code() != 0) {
                std::cout << "服务端返回错误码: " << resp.error_code() << "\n";
            }
            break;
        } 
        else if (pr == ParseResult::kNeedMore) continue;
        else {
            std::cout << "解码响应失败: " << static_cast<int>(pr) << "\n";
            resp.set_error_code(kNetworkError);
            return false;
        }
    }

    return true;
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

    rpc::RpcMessage resp;
    resp.set_type(rpc::MESSAGE_TYPE_RESPONSE);
    resp.set_request_id(msg.request_id());

    const int re_try = 2;
    int lastErr = 0;
    for(int i = 0; i < re_try; i++){
        if(fd_ == -1 && !connect()){
            lastErr = kNetworkError;
            continue;
        }

        if(!send(sendBuf)){
            lastErr = kNetworkError;
            continue;
        }

        if(!recv(resp)){
            lastErr = resp.error_code();
            continue;
        }
        else return resp;
    }
    resp.set_error_code(lastErr != 0 ? lastErr : kNetworkError);
    
    return resp;
}

rpc::EchoResponse RpcClient::Echo(const std::string& text) {
    rpc::EchoRequest req;
    req.set_text(text);
    std::string payload;
    req.SerializeToString(&payload);

    rpc::RpcMessage resp = call("EchoService", "Echo", payload);

    if(resp.error_code() != 0){
        std::cout << "RPC 调用失败, error_code=" << resp.error_code() << "\n";
    }

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

    if(resp.error_code() != 0){
       std::cout << "RPC 调用失败, error_code=" << resp.error_code() << "\n";
    }

    rpc::AddResponse result;
    result.ParseFromString(resp.payload());
    return result.sum();
}