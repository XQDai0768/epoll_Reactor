#pragma once

// ---- 服务端业务错误码 ----
constexpr int kOk = 0;
constexpr int kServiceNotFound = 1;
constexpr int kMethodNotFound = 2;
constexpr int kBadRequest = 3;

// ---- 客户端本地传输错误码（100 起，区分开）----
constexpr int kNetworkError = 100;   // 收发失败 / 连接断开
constexpr int kTimeout = 101;        // 超时