// RpcCodec 单元测试：覆盖 round-trip / 半包 / 粘包 / 非法长度 / 非法 payload
#include "service.pb.h"
#include "buffer.h"
#include "rpc_codec.h"

#include <arpa/inet.h>   // htonl
#include <cstdint>
#include <iostream>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const std::string& name) {
    if (ok) {
        std::cout << "[PASS] " << name << "\n";
        ++g_pass;
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++g_fail;
    }
}

static void checkResult(ParseResult actual, ParseResult expected, const std::string& name) {
    if (actual == expected) {
        std::cout << "[PASS] " << name << "\n";
        ++g_pass;
    } else {
        std::cout << "[FAIL] " << name
                  << " (expect " << static_cast<int>(expected)
                  << ", got " << static_cast<int>(actual) << ")\n";
        ++g_fail;
    }
}

static rpc::RpcMessage makeMessage(uint64_t id,
                                   const std::string& service,
                                   const std::string& method,
                                   const std::string& payload) {
    rpc::RpcMessage msg;
    msg.set_type(rpc::MESSAGE_TYPE_REQUEST);
    msg.set_request_id(id);
    msg.set_service(service);
    msg.set_method(method);
    msg.set_payload(payload);
    msg.set_error_code(0);
    return msg;
}

static bool sameMessage(const rpc::RpcMessage& a, const rpc::RpcMessage& b) {
    return a.type() == b.type()
        && a.request_id() == b.request_id()
        && a.service() == b.service()
        && a.method() == b.method()
        && a.payload() == b.payload()
        && a.error_code() == b.error_code();
}

// 1. 单条完整消息往返
static void testRoundTrip() {
    Buffer buf;
    RpcCodec codec;

    auto msg = makeMessage(1, "svc.user.UserService", "GetUser", "hello");
    codec.encode(msg, &buf);

    rpc::RpcMessage decoded;
    ParseResult r = codec.decode(&buf, &decoded);
    check(r == ParseResult::kOk && sameMessage(msg, decoded), "round-trip 编解码往返一致");
}

// 2. 半包：头不完整、体不完整
static void testHalfPacket() {
    Buffer buf;
    RpcCodec codec;

    auto msg = makeMessage(2, "svc", "Half", "half-packet-body");

    std::string payload;
    msg.SerializeToString(&payload);
    uint32_t len = static_cast<uint32_t>(payload.size());
    uint32_t be = htonl(len);
    const char* header = reinterpret_cast<const char*>(&be);

    rpc::RpcMessage decoded;

    // 2a. 只放 2 字节头
    buf.append(header, 2);
    checkResult(codec.decode(&buf, &decoded), ParseResult::kNeedMore, "半包(仅2字节头) -> kNeedMore");

    // 2b. 补全头，但 body 只放一半
    buf.append(header + 2, 2);
    buf.append(payload.data(), payload.size() / 2);
    checkResult(codec.decode(&buf, &decoded), ParseResult::kNeedMore, "半包(头全+半个body) -> kNeedMore");

    // 2c. 补全剩余 body
    buf.append(payload.data() + payload.size() / 2, payload.size() - payload.size() / 2);
    checkResult(codec.decode(&buf, &decoded), ParseResult::kOk, "半包补齐后 -> kOk");
    check(sameMessage(msg, decoded), "半包补齐后内容一致");
}

// 3. 粘包：两条消息连续放入同一个 Buffer
static void testStickyPacket() {
    Buffer buf;
    RpcCodec codec;

    auto m1 = makeMessage(3, "svc", "M1", "first");
    auto m2 = makeMessage(4, "svc", "M2", "second-message");

    codec.encode(m1, &buf);
    codec.encode(m2, &buf);

    rpc::RpcMessage d1, d2;
    check(codec.decode(&buf, &d1) == ParseResult::kOk && sameMessage(m1, d1), "粘包第1条解出");
    check(codec.decode(&buf, &d2) == ParseResult::kOk && sameMessage(m2, d2), "粘包第2条解出");

    rpc::RpcMessage d3;
    checkResult(codec.decode(&buf, &d3), ParseResult::kNeedMore, "粘包解完后 -> kNeedMore");
}

// 4. 非法长度：长度字段过大，应拒绝
static void testInvalidLength() {
    Buffer buf;
    RpcCodec codec;

    uint32_t bad = htonl(0xFFFFFFFFu);
    buf.append(&bad, sizeof(bad));

    rpc::RpcMessage decoded;
    checkResult(codec.decode(&buf, &decoded), ParseResult::kInvalidLength, "超大长度头 -> kInvalidLength");
}

// 5. 非法 payload：长度合法但内容不是 protobuf，应报解析错误
static void testParseError() {
    Buffer buf;
    RpcCodec codec;

    uint32_t len = htonl(1u);
    buf.append(&len, sizeof(len));
    const char garbage = 0x00;   // field number 0 非法，解析必然失败
    buf.append(&garbage, 1);

    rpc::RpcMessage decoded;
    checkResult(codec.decode(&buf, &decoded), ParseResult::kParseError, "非法payload -> kParseError");
}

int main() {
    testRoundTrip();
    testHalfPacket();
    testStickyPacket();
    testInvalidLength();
    testParseError();

    std::cout << "\n结果: PASS=" << g_pass << " FAIL=" << g_fail << "\n";
    return g_fail == 0 ? 0 : 1;
}
