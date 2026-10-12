# 基于Reactor模型Protobuf协议的简易RPC框架

一个基于 **epoll + Reactor** 的 C++17 服务端 / 客户端学习项目，目标是逐步演进为完整的简易 RPC 框架。

当前版本 **v7**：在 Reactor 网络层之上完成了 Protobuf 编解码、服务注册与分发、以及客户端同步 stub。服务端可以注册多个服务与多个方法，客户端通过强类型接口发起调用，请求 / 响应端到端已经跑通。

---

## 项目简介

本项目从一个阻塞 echo server 开始，逐步加入：

- epoll 多路复用
- Reactor 事件分发模型
- 非阻塞 I/O 与边缘触发
- Buffer 粘包/拆包处理
- ThreadPool 业务线程池
- timerfd 定时器
- 空闲连接超时
- SIGINT / SIGTERM 优雅退出
- Protobuf 编解码
- RPC 服务注册、分发与客户端 stub
- 客户端调用超时与断线重连

---

## 版本清单

| 版本 | 名称 | 相比上一版本的主要更新 |
| --- | --- | --- |
| v0 | 阻塞 echo server | 掌握 socket、bind、listen、accept、read、write、close，实现单连接回显 |
| v1 | epoll LT 单文件 | 引入 epoll_create/ctl/wait，支持多连接；新增非阻塞 clientfd 和 4 字节长度协议 |
| v2 | Reactor OOP | 拆出 EventLoop、Channel、Acceptor、TcpConnection、Buffer，建立事件分发与回调体系 |
| v3 | TcpServer + ET | 新增 TcpServer 抽象；启用边缘触发；通过 runInLoop 延迟清理连接 |
| v4 | ThreadPool | IO 线程与 Worker 线程分离；eventfd 跨线程唤醒；weak_ptr 管理异步连接生命周期 |
| v5 | 优雅退出 + 信号处理 | 忽略 SIGPIPE；处理 SIGINT/SIGTERM；Acceptor/TcpServer 停止流程与资源释放顺序 |
| v6 | TimerQueue | 引入 timerfd 定时器；支持一次性/周期任务、取消任务和空闲连接超时关闭 |
| v7 | Protobuf RPC 化 | 定义 RpcMessage 信封；RpcCodec 编解码；Service 基类与方法注册表；Dispatcher 服务路由；EchoService / CalculatorService；客户端同步 stub；客户端超时与断线重连 |

当前最新版本：**v7**

下一阶段：**v8 异步并发客户端（request_id 多路复用）**

---

## 已实现功能

### 网络层

- 基于 `epoll_wait` 的事件循环
- `EventLoop` / `Channel` / `Acceptor` / `TcpConnection` / `TcpServer` 分层设计
- 非阻塞 socket 与 `EAGAIN` 处理
- `EPOLLET` 边缘触发 + 循环读写
- `Buffer` 自动扩容与半包 / 粘包处理
- 输出缓冲区与 `EPOLLOUT` 写事件处理

### 并发与定时

- 线程池：IO 线程与 Worker 线程分离
- Worker 完成任务后通过 `runInLoop` 回到 IO 线程发送
- `eventfd` 跨线程唤醒
- `timerfd` 定时器与 `TimerQueue`
- 空闲连接超时关闭
- `SIGPIPE` 忽略
- `SIGINT` / `SIGTERM` 优雅退出
- `shared_ptr` / `weak_ptr` 管理异步任务中的连接生命周期

### 生命周期

- 连接延迟清理

### RPC

- Protobuf 消息定义与编解码（`RpcCodec`）
- 统一消息信封 `RpcMessage`（type / request_id / service / method / payload / error_code）
- `Service` 基类 + 方法注册表，服务只需注册方法名到 Handler
- `Dispatcher` 按 service 路由到 Service，按 method 分发到 Handler
- `EchoService`、`CalculatorService` 两个示例服务
- 客户端同步 stub：`Echo()` / `Add()` 强类型接口
- 客户端调用超时（`SO_RCVTIMEO`）与失败断线重连

---

## 目录结构

```text
.
├── server/
│   ├── include/          # 服务端头文件
│   ├── src/              # 服务端实现
│   ├── proto/            # Protobuf 消息与服务定义（service / echo / calculator）
│   ├── test/             # 编解码测试
│   ├── olds/             # 历史版本快照
│   └── CMakeLists.txt
├── client/
│   ├── include/          # 客户端头文件（RpcClient）
│   ├── src/              # 客户端实现与示例 main
│   └── CMakeLists.txt
├── docs/                 # 学习笔记、技能清单、错误记录
├── README.md
└── .gitignore
```

核心类：

| 类 | 职责 |
| --- | --- |
| `EventLoop` | 管理 epoll、事件循环、跨线程任务和定时器 |
| `Channel` | 封装一个 fd、关心的事件和事件回调 |
| `Acceptor` | 监听 listenfd，接受新连接 |
| `TcpConnection` | 管理一条连接的读写、缓冲区、协议解析和生命周期 |
| `TcpServer` | 管理 Acceptor、连接集合和业务分发 |
| `Buffer` | 动态缓冲区，负责粘包/拆包 |
| `ThreadPool` | 执行耗时业务任务 |
| `TimerQueue` | 基于 timerfd 的定时任务管理 |
| `RpcCodec` | Protobuf 消息的编解码（4 字节长度头 + payload） |
| `Service` | 服务基类，维护 method 到 Handler 的注册表 |
| `Dispatcher` | 按 service / method 把请求路由到对应 Handler |
| `EchoService` / `CalculatorService` | 示例服务实现 |
| `RpcClient` | 客户端：建连、编码发送、接收解码、超时与重连 |

---

## 构建与运行

### 环境要求

- Linux 环境
- CMake 3.10+
- 支持 C++17 的 GCC / G++
- Protobuf（`protobuf-compiler` + `libprotobuf-dev`）
- pthread

macOS 本地没有 epoll，建议在 Docker / Linux 容器中构建运行。仓库上层的 `../Dockerfile`（镜像 `web_env`）已装好 g++、cmake、gdb、protobuf 等依赖，代码挂载到容器的 `/apps`。

### 构建服务端

```bash
cmake -S server -B server/build
cmake --build server/build -j
```

### 运行服务端

```bash
./server/build/server 8888
```

参数：

- `8888`：监听端口

启动后可以通过 `Ctrl+C` 或 `kill -TERM <pid>` 触发优雅退出。

### 构建与运行客户端

```bash
cmake -S client -B client/build
cmake --build client/build -j
./client/build/client 127.0.0.1 8888
```

客户端会依次调用 `EchoService.Echo` 和 `CalculatorService.Add` 并打印结果。

> 客户端与服务端共用 `buffer.cpp`、`rpc_codec.cpp`，Protobuf 代码从 `server/proto/` 生成。

---

## RPC 协议

消息体是 Protobuf 序列化后的 `RpcMessage`（见 `server/proto/service.proto`），外面套一层 4 字节大端长度头用于分帧：

```text
+---------------------------+--------------------------------------+
| 4 字节长度，大端序         | Protobuf 序列化的 RpcMessage          |
+---------------------------+--------------------------------------+
```

长度字段只包含消息体长度，不包含长度头本身。

`RpcMessage` 字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `type` | enum | 消息类型：REQUEST / RESPONSE / HEARTBEAT |
| `request_id` | uint64 | 请求 ID，用于关联请求与响应 |
| `service` | string | 服务名，如 `EchoService` |
| `method` | string | 方法名，如 `Echo` |
| `payload` | bytes | 业务消息序列化后的数据（如 `EchoRequest`） |
| `error_code` | int32 | 错误码，0 表示成功 |

业务数据（`EchoRequest`、`AddRequest` 等）单独定义在 `echo.proto`、`calculator.proto` 中，作为 `payload` 承载。

错误码分层（见 `server/include/error_code.h`）：

- `0 ~ 3`：服务端业务错误码（成功 / 服务不存在 / 方法不存在 / 请求非法）
- `100+`：客户端本地传输错误码（网络错误 / 超时）

---

## 设计要点

- IO 线程只负责网络读写和事件分发，Worker 线程只负责业务计算。
- Worker 不直接操作 fd、`Channel` 或 `Buffer`，完成后通过 `runInLoop()` 回到 IO 线程。
- `eventfd` 用于唤醒阻塞中的 `epoll_wait`。
- 异步任务通过 `weak_ptr` 检查连接是否仍然存活。
- `timerfd` 被注册进 epoll，定时器到期后以事件方式处理。
- 收到完整消息后重置空闲定时器，超时则关闭连接。
- 业务与网络解耦：`Service` 只关心收到 RpcMessage 后填回 RpcMessage，不接触 socket。
- 客户端把一次调用封装为 `call()`：编码发送到接收解码，最后返回响应信封；上层 stub 负责 payload 的序列化与反序列化。

---

## 当前限制

- 客户端是**同步阻塞**的：一次只能有一个请求在途，`call()` 返回前调用线程一直等待。
- 客户端超时基于 `SO_RCVTIMEO`，超时粒度为整条连接，而非单个请求。
- 未做 `request_id` 匹配校验，超时重连场景下仍可能读到错配响应。
- 失败即重试，未做幂等 / 去重，属于 at-least-once 语义。
- 没有连接池与连接复用，没有服务注册发现与负载均衡。
- 没有心跳 / 保活机制。
- 没有统一的日志系统和配置模块。
- 定时器使用 `std::multimap`，适合学习阶段，不适合海量定时器。
- 单元测试、压力测试体系还不完整。

---

## 待实现模块

- [x] Protobuf 协议编解码
- [x] 请求 / 响应消息定义
- [x] RPC 服务注册与分发
- [x] RPC 客户端与 stub
- [x] 请求超时与断线重连
- [ ] 异步并发客户端（request_id 多路复用）—— 下一阶段 v8
- [ ] 每次请求独立的超时管理（基于 TimerQueue）
- [ ] 客户端连接池与连接复用
- [ ] 心跳 / 保活
- [ ] 服务注册与发现、负载均衡
- [ ] 统一日志系统
- [ ] 配置模块
- [ ] 定时器优化：最小堆或时间轮
- [ ] Google Test 单元测试
- [ ] 压力测试与性能数据
- [ ] Dockerfile 与容器化构建
- [ ] HTTP 协议或简单网关能力

---

## 学习文档

更多设计思路、自查问题和学习笔记见：

- `docs/LEARNING_NOTES.md`
- `docs/SKILL_CHECKLIST.md`
- `docs/ERROR.md`

---

## 说明

这是一个学习项目，代码会持续迭代和重构。当前阶段优先保证“理解原理、掌握设计”，性能优化和工程化细节会逐步补充。
