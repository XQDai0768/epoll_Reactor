// 最简单的 RPC 客户端：连上服务端，发一条 RpcMessage，收一条响应
#include "service.pb.h"
#include "buffer.h"
#include "rpc_codec.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "usage: client <ip> <port>\n";
        return -1;
    }

    const char* ip = argv[1];
    int port = std::atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); return -1; }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return -1;
    }

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    // 1. 构造请求
    rpc::RpcMessage req;
    req.set_type(rpc::MESSAGE_TYPE_REQUEST);
    req.set_request_id(1);
    req.set_service("demo.Service");
    req.set_method("Echo");
    req.set_payload("hello rpc");
    req.set_error_code(0);

    // 2. 编码：4 字节长度头 + protobuf
    Buffer sendBuf;
    RpcCodec codec;
    codec.encode(req, &sendBuf);

    // 3. 发送（循环发，确保发完）
    size_t sent = 0;
    while (sent < sendBuf.readableBytes()) {
        ssize_t n = send(fd, sendBuf.peek() + sent, sendBuf.readableBytes() - sent, 0);
        if (n <= 0) { perror("send"); close(fd); return -1; }
        sent += static_cast<size_t>(n);
    }
    std::cout << "已发送请求 (" << sent << " 字节)\n";

    // 4. 接收并解码（循环读，处理响应半包）
    Buffer recvBuf;
    rpc::RpcMessage resp;
    while (true) {
        char tmp[65536];
        ssize_t r = recv(fd, tmp, sizeof(tmp), 0);
        if (r == 0) { std::cout << "服务端关闭了连接\n"; break; }
        if (r < 0) { perror("recv"); break; }

        recvBuf.append(tmp, static_cast<size_t>(r));

        ParseResult pr = codec.decode(&recvBuf, &resp);
        if (pr == ParseResult::kOk) {
            std::cout << "收到响应:\n" << resp.DebugString();
            break;
        } else if (pr == ParseResult::kNeedMore) {
            continue;
        } else {
            std::cout << "解码响应失败: " << static_cast<int>(pr) << "\n";
            break;
        }
    }

    close(fd);
    return 0;
}
