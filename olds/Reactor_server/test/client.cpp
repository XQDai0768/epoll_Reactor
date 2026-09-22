#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const size_t kMaxMessageBody = 65536;

static bool connect_server(const char* host, uint16_t port, int& fd) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        fd = -1;
        return false;
    }

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        close(fd);
        fd = -1;
        return false;
    }

    return true;
}

static void close_server(int& fd) {
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

static bool send_all(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, data + sent, len - sent);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

static bool recv_all(int fd, char* data, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, data + got, len - got);
        if (n <= 0) {
            return false;
        }
        got += static_cast<size_t>(n);
    }
    return true;
}

static std::string make_packet(const std::string& body) {
    uint32_t body_len = static_cast<uint32_t>(body.size());
    uint32_t net_len = htonl(body_len);

    std::string packet;
    packet.append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    packet.append(body);
    return packet;
}

static bool send_packet(int fd, const std::string& packet) {
    return send_all(fd, packet.data(), packet.size());
}

static bool send_message(int fd, const std::string& body) {
    std::string packet = make_packet(body);
    return send_packet(fd, packet);
}

static bool recv_message(int fd, std::string& body) {
    uint32_t net_len = 0;
    if (!recv_all(fd, reinterpret_cast<char*>(&net_len), sizeof(net_len))) {
        return false;
    }

    uint32_t body_len = ntohl(net_len);
    if (body_len > kMaxMessageBody) {
        return false;
    }

    body.assign(body_len, '\0');
    if (body_len > 0) {
        if (!recv_all(fd, body.data(), body_len)) {
            return false;
        }
    }
    return true;
}

static bool ensure_connected(const char* host, uint16_t port, int& fd) {
    if (fd != -1) {
        return true;
    }

    if (!connect_server(host, port, fd)) {
        std::cerr << "连接失败: " << host << ':' << port << '\n';
        return false;
    }

    std::cout << "已重新连接 " << host << ':' << port << '\n';
    return true;
}

static void print_separator() {
    std::cout << "----------------------------------------\n";
}

static void run_free_send(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    std::cout << "自由发送模式：输入一行发送，输入 back 返回菜单，quit 退出程序。\n";
    std::string line;
    while (std::cout << "send> " && std::getline(std::cin, line)) {
        if (line == "back") {
            break;
        }
        if (line == "quit") {
            close_server(fd);
            std::exit(0);
        }
        if (line.empty()) {
            continue;
        }

        if (!send_message(fd, line)) {
            std::cerr << "发送失败，连接可能已关闭\n";
            close_server(fd);
            break;
        }

        std::string echo;
        if (!recv_message(fd, echo)) {
            std::cerr << "服务端关闭了连接\n";
            close_server(fd);
            break;
        }

        std::cout << "echo: " << echo << '\n';
    }
}

static void run_complete_message(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    const std::string body = "hello";
    if (!send_message(fd, body)) {
        std::cerr << "发送失败\n";
        close_server(fd);
        return;
    }

    std::string echo;
    if (!recv_message(fd, echo)) {
        std::cerr << "服务端关闭了连接\n";
        close_server(fd);
        return;
    }

    if (echo == body) {
        std::cout << "[PASS] 完整消息回显正确: " << echo << '\n';
    } else {
        std::cout << "[FAIL] 完整消息期望 '" << body << "'，实际 '" << echo << "'\n";
    }
}

static void run_half_packet(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    const std::string body = "hello";
    std::string packet = make_packet(body);

    if (packet.size() < 3) {
        return;
    }

    if (!send_all(fd, packet.data(), 2)) {
        std::cerr << "发送前半部分失败\n";
        close_server(fd);
        return;
    }

    std::cout << "已发送前 2 字节，等待 1 秒...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!send_all(fd, packet.data() + 2, packet.size() - 2)) {
        std::cerr << "发送剩余部分失败\n";
        close_server(fd);
        return;
    }

    std::string echo;
    if (!recv_message(fd, echo)) {
        std::cerr << "服务端关闭了连接\n";
        close_server(fd);
        return;
    }

    if (echo == body) {
        std::cout << "[PASS] 半包场景回显正确: " << echo << '\n';
    } else {
        std::cout << "[FAIL] 半包期望 '" << body << "'，实际 '" << echo << "'\n";
    }
}

static void run_sticky_packet(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    std::string packet;
    packet += make_packet("hello");
    packet += make_packet("world");

    if (!send_packet(fd, packet)) {
        std::cerr << "发送失败\n";
        close_server(fd);
        return;
    }

    std::string first;
    std::string second;
    bool ok = recv_message(fd, first) && recv_message(fd, second);

    if (!ok) {
        std::cerr << "接收失败，服务端可能关闭了连接\n";
        close_server(fd);
        return;
    }

    if (first == "hello" && second == "world") {
        std::cout << "[PASS] 粘包正确拆成两条: " << first << " / " << second << '\n';
    } else {
        std::cout << "[FAIL] 粘包期望 hello / world，实际 " << first << " / " << second << '\n';
    }
}

static void run_multi_connection(const char* host, uint16_t port) {
    std::vector<int> conns;
    bool ok = true;

    for (int i = 0; i < 5; ++i) {
        int cfd = -1;
        if (!connect_server(host, port, cfd)) {
            std::cerr << "连接 " << i + 1 << " 失败\n";
            ok = false;
            break;
        }
        conns.push_back(cfd);
    }

    if (!ok) {
        for (int cfd : conns) {
            close(cfd);
        }
        return;
    }

    for (size_t i = 0; i < conns.size(); ++i) {
        std::string body = "conn" + std::to_string(i + 1);
        if (!send_message(conns[i], body)) {
            std::cerr << "连接 " << i + 1 << " 发送失败\n";
            ok = false;
        }
    }

    for (size_t i = 0; i < conns.size() && ok; ++i) {
        std::string echo;
        if (!recv_message(conns[i], echo)) {
            std::cerr << "连接 " << i + 1 << " 接收失败\n";
            ok = false;
            break;
        }

        std::string expected = "conn" + std::to_string(i + 1);
        if (echo != expected) {
            std::cout << "[FAIL] 连接 " << i + 1 << " 期望 " << expected << "，实际 " << echo << '\n';
            ok = false;
        }
    }

    if (ok) {
        std::cout << "[PASS] 5 个并发连接均正常回显\n";
    }

    for (int cfd : conns) {
        close(cfd);
    }
}

static void run_invalid_length(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    uint32_t bad_len = htonl(65536);
    std::string packet;
    packet.append(reinterpret_cast<const char*>(&bad_len), sizeof(bad_len));

    if (!send_packet(fd, packet)) {
        std::cerr << "发送失败\n";
        close_server(fd);
        return;
    }

    std::string echo;
    if (!recv_message(fd, echo)) {
        std::cout << "[PASS] 非法长度连接被服务端关闭\n";
        close_server(fd);
        return;
    }

    std::cout << "[FAIL] 非法长度不应收到回显，但收到 '" << echo << "'\n";
}

static void run_large_message(const char* host, uint16_t port, int& fd) {
    if (!ensure_connected(host, port, fd)) {
        return;
    }

    std::string body(1000, 'A');
    if (!send_message(fd, body)) {
        std::cerr << "发送失败\n";
        close_server(fd);
        return;
    }

    std::string echo;
    if (!recv_message(fd, echo)) {
        std::cerr << "服务端关闭了连接\n";
        close_server(fd);
        return;
    }

    if (echo == body) {
        std::cout << "[PASS] 1000 字节大消息回显正确\n";
    } else {
        std::cout << "[FAIL] 大消息长度不一致，期望 " << body.size() << "，实际 " << echo.size() << '\n';
    }
}

static void run_close_reconnect(const char* host, uint16_t port, int& fd) {
    close_server(fd);

    if (!connect_server(host, port, fd)) {
        std::cerr << "重连失败\n";
        return;
    }

    std::cout << "已关闭旧连接并重新连接\n";

    if (!send_message(fd, "alive")) {
        std::cerr << "发送失败\n";
        close_server(fd);
        return;
    }

    std::string echo;
    if (!recv_message(fd, echo)) {
        std::cerr << "服务端关闭了连接\n";
        close_server(fd);
        return;
    }

    if (echo == "alive") {
        std::cout << "[PASS] 关闭重连后服务端仍可服务\n";
    } else {
        std::cout << "[FAIL] 关闭重连后回显异常: " << echo << '\n';
    }
}

int main(int argc, char* argv[]) {
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (argc > 2) ? static_cast<uint16_t>(atoi(argv[2])) : 8888;

    std::cout << "测试客户端，目标 " << host << ':' << port << '\n';

    int fd = -1;
    std::string choice;

    while (true) {
        print_separator();
        std::cout << "1. 自由发送消息\n"
                  << "2. 完整消息检测\n"
                  << "3. 半包检测\n"
                  << "4. 粘包检测\n"
                  << "5. 多连接检测\n"
                  << "6. 非法长度检测\n"
                  << "7. 大消息检测\n"
                  << "8. 关闭并重连检测\n"
                  << "0. 退出\n"
                  << "选择: ";

        if (!std::getline(std::cin, choice)) {
            break;
        }

        if (choice == "1") {
            run_free_send(host, port, fd);
        } else if (choice == "2") {
            run_complete_message(host, port, fd);
        } else if (choice == "3") {
            run_half_packet(host, port, fd);
        } else if (choice == "4") {
            run_sticky_packet(host, port, fd);
        } else if (choice == "5") {
            run_multi_connection(host, port);
        } else if (choice == "6") {
            run_invalid_length(host, port, fd);
        } else if (choice == "7") {
            run_large_message(host, port, fd);
        } else if (choice == "8") {
            run_close_reconnect(host, port, fd);
        } else if (choice == "0") {
            break;
        } else {
            std::cout << "无效选择\n";
        }
    }

    close_server(fd);
    return 0;
}
