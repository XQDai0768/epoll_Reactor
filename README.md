# 基于Reactor模型和Protobuf协议的简易RPC框架

一个基于 **epoll + Reactor + Protobuf** 的 C++17 **Mini RPC Framework**，采用非阻塞 I/O、边缘触发、事件驱动和线程池，实现 RPC 编解码、服务注册与分发、同步/异步客户端调用、请求级超时与优雅退出。

当前版本 **v8**：在 v7 的 Protobuf RPC 基础上完成非阻塞连接与异步客户端，支持基于 `request_id` 的单连接多请求并发、响应乱序匹配和每请求独立超时。

## 技术栈

- 语言与标准：C++17
- 网络与 I/O：Linux epoll、非阻塞 I/O、EPOLLET、eventfd、timerfd
- 架构与并发：Reactor、IO 线程 + 业务线程池、RAII、shared_ptr / weak_ptr
- 协议：Protobuf、4 字节大端长度分帧
- 构建与环境：CMake、GCC/G++、Docker、Git

---

## 项目简介

项目采用渐进式架构演进，覆盖从 socket 编程到完整 RPC 调用链的核心能力：

- epoll 多路复用
- Reactor 事件分发模型
- 非阻塞 I/O 与边缘触发
- Buffer 粘包/拆包处理
- ThreadPool 业务线程池
- timerfd 定时器
- 空闲连接超时
- SIGINT / SIGTERM 优雅退出
- Protobuf 编解码
- RPC 服务注册、分发与同步客户端 stub
- 非阻塞异步客户端与 request_id 多路复用
- 请求级超时与失败清理
- CMake 构建与 Docker 化开发环境

---

## 版本清单

| 版本 | 名称 | 相比上一版本的主要更新 |
| --- | --- | --- |
| v0 | 阻塞 echo server | 实现 socket、bind、listen、accept、read、write、close，完成单连接回显 |
| v1 | epoll LT 单文件 | 引入 epoll_create/ctl/wait，支持多连接；新增非阻塞 clientfd 和 4 字节长度协议 |
| v2 | Reactor OOP | 拆出 EventLoop、Channel、Acceptor、TcpConnection、Buffer，建立事件分发与回调体系 |
| v3 | TcpServer + ET | 新增 TcpServer 抽象；启用边缘触发；通过 runInLoop 延迟清理连接 |
| v4 | ThreadPool | IO 线程与 Worker 线程分离；eventfd 跨线程唤醒；weak_ptr 管理异步连接生命周期 |
| v5 | 优雅退出 + 信号处理 | 忽略 SIGPIPE；处理 SIGINT/SIGTERM；Acceptor/TcpServer 停止流程与资源释放顺序 |
| v6 | TimerQueue | 引入 timerfd 定时器；支持一次性/周期任务、取消任务和空闲连接超时关闭 |
| v7 | Protobuf RPC 化 | 定义 RpcMessage 信封；RpcCodec 编解码；Service 基类与方法注册表；Dispatcher 服务路由；EchoService / CalculatorService；客户端同步 stub；客户端超时与断线重连 |
| v8 | 异步并发客户端 | 非阻塞 connect；request_id pending 映射；单连接多请求并发；响应乱序匹配；每请求独立 TimerQueue 超时；异步客户端示例 |

当前最新版本：**v8**

下一阶段：**v9 连接复用与服务端多 Reactor 演进（规划）**

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
- 同步客户端：基于 `SO_RCVTIMEO` 的调用超时与失败断线重连
- 异步客户端：非阻塞 connect、`request_id` 多路复用、请求级超时与 pending 清理

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
│   ├── include/          # 客户端头文件（RpcClient / RpcChannel）
│   ├── src/              # 同步/异步客户端实现与示例 main
│   └── CMakeLists.txt
├── docs/                 # 设计笔记、技能清单、错误记录
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
| `RpcClient` | 同步客户端：建连、编码发送、接收解码、超时与重连 |
| `RpcChannel` | 异步客户端：非阻塞 connect、request_id pending 映射、请求级超时 |

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
./client/build/async_client 127.0.0.1 8888
```

`client` 为同步客户端，会依次调用 `EchoService.Echo` 和 `CalculatorService.Add`；`async_client` 为异步客户端，会在同一连接上并发发送多个 Echo 请求，并按 `request_id` 匹配乱序返回的响应。

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
- 同步客户端把一次调用封装为 `call()`：编码发送到接收解码，最后返回响应信封；上层 stub 负责 payload 的序列化与反序列化。
- 异步客户端通过 `request_id` 维护 pending 表，单连接可同时存在多个在途请求；响应按 id 查找回调，不依赖到达顺序。
- 每个异步请求由 `TimerQueue` 独立计时，响应到达时取消定时器；超时与响应竞争时以先删除 pending 的一方为准，避免重复回调。

---

## 当前限制

- 服务端当前是单 IO Reactor + 业务线程池，尚未实现 main/sub 多 Reactor。
- 异步客户端当前使用单连接多路复用，尚未实现连接池与多连接负载分摊。
- 客户端发送路径尚未为大 payload 实现完整的 EPOLLOUT 发送队列；当前按单次 `send` 完成处理，遇到 `EAGAIN` 会按网络错误处理。
- 同步客户端仍保留 `SO_RCVTIMEO` 作为调用超时；异步客户端使用 TimerQueue 做请求级超时。
- 失败调用未做幂等 / 去重，重试语义为 at-least-once。
- 没有心跳 / 保活、统一日志、配置模块、服务注册发现与负载均衡。
- 定时器使用 `std::multimap`，适合中小规模定时任务，尚未替换为最小堆或时间轮。
- 单元测试、压力测试和性能数据仍待完善。

---

## 待实现模块

- [x] Protobuf 协议编解码
- [x] 请求 / 响应消息定义
- [x] RPC 服务注册与分发
- [x] RPC 客户端与 stub
- [x] 请求超时与断线重连
- [x] 异步并发客户端（request_id 多路复用）
- [x] 每次请求独立的超时管理（基于 TimerQueue）
- [ ] 客户端发送缓冲与 EPOLLOUT 队列
- [ ] 客户端连接池与连接复用
- [ ] 服务端多 Reactor（main/sub Reactor）
- [ ] 心跳 / 保活
- [ ] 服务注册与发现、负载均衡
- [ ] 统一日志系统
- [ ] 配置模块
- [ ] 定时器优化：最小堆或时间轮
- [ ] Google Test 单元测试
- [ ] 压力测试与性能数据
- [ ] 仓库内 Dockerfile 与一键启动脚本
- [ ] HTTP 协议或简单网关能力

---

## 项目文档

设计思路、自查问题与问题记录见 `docs/` 目录。

---

## 说明

本项目持续迭代，当前重点是完善异步 RPC 客户端、自动化测试与性能数据；后续将演进连接复用、多 Reactor、心跳与可观测性。
