#pragma once
#include "service.pb.h"

class Buffer;

enum class ParseResult {
    kOk,            // 成功解析出一个完整消息
    kNeedMore,      // 数据不足，等下次读再试
    kInvalidLength, // 长度字段非法（过大），应断开连接
    kParseError     // payload 不是合法 protobuf，应断开连接
};

class RpcCodec {
public:
    void encode(const rpc::RpcMessage& msg, Buffer* buf);
    ParseResult decode(Buffer* buf, rpc::RpcMessage* msg);
};