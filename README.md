# 基于Reactor模型Protobuf协议的简易RPC框架

一个基于 **epoll + Reactor** 的 C++17 服务端学习项目，目标是逐步演进为完整的简易 RPC 框架。

当前阶段已经完成：非阻塞 I/O、边缘触发、单线程 Reactor、多线程业务处理、定时器、空闲连接管理和优雅退出。Protobuf 协议与 RPC 调度模块仍在开发中。

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

当前网络层已经具备一个可扩展服务端的骨架，后续将在此基础上替换协议并实现 RPC 能力。

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

当前最新版本：**v6**

下一阶段：**Protobuf Codec**

---

## 已实现功能

- 基于 `epoll_wait` 的事件循环
- `EventLoop` / `Channel` / `Acceptor` / `TcpConnection` / `TcpServer` 分层设计
- 非阻塞 socket 与 `EAGAIN` 处理
- `EPOLLET` 边缘触发 + 循环读写
- 4 字节长度头 + 消息体的临时协议
- `Buffer` 自动扩容与半包/粘包处理
- 线程池：IO 线程与 Worker 线程分离
- Worker 完成任务后通过 `runInLoop` 回到 IO 线程发送
- `eventfd` 跨线程唤醒
- `timerfd` 定时器与 `TimerQueue`
- 空闲连接超时关闭
- `SIGPIPE` 忽略
- `SIGINT` / `SIGTERM` 优雅退出
- `shared_ptr` / `weak_ptr` 管理异步任务中的连接生命周期

---

## 目录结构

```text
.
├── server/
│   ├── include/          # 服务端头文件
│   ├── src/              # 服务端实现
│   ├── olds/             # 历史版本快照
│   └── CMakeLists.txt
├── client/               # 客户端目录，当前预留
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
| `TcpConnection` | 管理一条连接的读写、缓冲区和生命周期 |
| `TcpServer` | 管理 Acceptor 和连接集合 |
| `Buffer` | 动态缓冲区，负责粘包/拆包 |
| `ThreadPool` | 执行耗时业务任务 |
| `TimerQueue` | 基于 timerfd 的定时任务管理 |

---

## 构建与运行

### 环境要求

- Linux 环境
- CMake 3.10+
- 支持 C++17 的 GCC / G++
- pthread

macOS 本地没有 epoll，建议在 Docker / Linux 容器中构建运行。

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

---

## 当前协议

当前版本使用简单的临时协议：

```text
+---------------------------+------------------+
| 4 字节长度，大端序         | 消息体            |
+---------------------------+------------------+
```

长度字段只包含消息体长度，不包含长度头本身。服务端当前按相同格式回显。

> 该协议是学习阶段的临时方案，后续将替换为 Protobuf 消息编码。

---

## 设计要点

- IO 线程只负责网络读写和事件分发，Worker 线程只负责业务计算。
- Worker 不直接操作 fd、`Channel` 或 `Buffer`，完成后通过 `runInLoop()` 回到 IO 线程。
- `eventfd` 用于唤醒阻塞中的 `epoll_wait`。
- 异步任务通过 `weak_ptr` 检查连接是否仍然存活。
- `timerfd` 被注册进 epoll，定时器到期后以事件方式处理。
- 收到完整消息后重置空闲定时器，超时则关闭连接。

---

## 当前限制

- 协议仍然是手写 4 字节长度 + 原始数据，尚未使用 Protobuf。
- 还没有正式的客户端和服务注册/分发机制。
- 定时器使用 `std::multimap`，适合学习阶段，不适合海量定时器。
- 日志仍以标准输出为主，还没有统一日志系统。
- 单元测试和压力测试体系还不完整。

---

## 待实现模块

- [ ] Protobuf 协议编解码
- [ ] 请求/响应消息定义
- [ ] RPC 服务注册与分发
- [ ] RPC 客户端与 stub
- [ ] 请求超时、心跳和重试
- [ ] 异步调用与 future/promise
- [ ] 统一日志系统
- [ ] 配置模块
- [ ] 定时器优化：最小堆或时间轮
- [ ] 单元测试与压力测试
- [ ] 客户端测试工具
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
