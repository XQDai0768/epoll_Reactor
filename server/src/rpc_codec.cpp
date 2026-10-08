#include "rpc_codec.h"
#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include "buffer.h"

void RpcCodec::encode(const rpc::RpcMessage& msg, Buffer* buf){
    std::string payload_;
    bool ok = msg.SerializeToString(&payload_);

    uint32_t len = payload_.size();
    
    uint32_t be = htonl(len);
    buf->append(&be, sizeof(be));            // 先写 4 字节头
    buf->append(payload_.data(), payload_.size());  // 再写 body
}

ParseResult RpcCodec::decode(Buffer* buf, rpc::RpcMessage* msg){
    if(buf->readableBytes() < 4) return ParseResult::kNeedMore;

    uint32_t len;
    memcpy(&len, buf->peek(), 4);
    len = ntohl(len);

    if(len > 65536) return ParseResult::kInvalidLength;

    if(buf->readableBytes() < 4 + len) return ParseResult::kNeedMore;

    bool flag_ = msg->ParseFromArray(buf->peek() + 4, len);
    if(flag_){
        buf->retrieve(4 + len);
        return ParseResult::kOk;
    }
    else return ParseResult::kParseError;
}