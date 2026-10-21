# tcp_chat_room
基于 C++17 的 Linux TCP 聊天室。

## 技术组成

聊天室所需的模块：

| 模块 | 文件 | 说明 |
|------|------|------|
| `lcl::net::ChatRoom` | `include/lcl/net/ChatRoom.hpp` | 聊天室核心：epoll Reactor 事件循环、非阻塞 IO、多房间消息路由、写背压广播、慢消费者踢出 |
| `lcl::net::Epoll` | `include/lcl/net/Epoll.hpp` | epoll 实例的 RAII 封装（添加/修改/删除监听、等待就绪事件） |
| `lcl::net::Socket` / `SocketAddress` / `TcpClient` | `include/lcl/net/` | RAII socket 封装、IPv4 地址、客户端连接 |
| `lcl::file::FileDescriptor` | `include/lcl/file/` | fd 的 RAII 封装（Socket/Epoll 依赖） |

协议为行文本（`\n` 结尾）：客户端连接后首行发送用户名完成注册，之后每行作为消息或命令。注册后自动进入默认房间 `lobby`，消息只在所属房间内广播，加入/离开有系统通知。

## 多房间

命令（在客户端输入后直接发送，服务端按行解析）：

| 命令 | 说明 |
|------|------|
| `/join <房间名>` | 切换房间；房间不存在则自动创建。房间名最长 32 字节 |
| `/rooms` | 列出所有房间及在线人数 |
| `/help` | 显示可用命令 |
| `/quit` | 客户端本地退出，不发送给服务端 |

每个房间成员相互隔离：`/join tech` 后，该客户端只收到 `tech` 房间的消息，也不再收到 `lobby` 的消息。房间无人时自动回收。

Reactor 设计要点：单线程事件循环（水平触发），无锁；连接按半包缓冲切行；发送时先直写，内核缓冲区满则剩余进入 `out_buf` 并挂 `EPOLLOUT`（写背压）；SIGINT/SIGTERM 与外部 `stop()` 统一通过 eventfd 唤醒 `epoll_wait` 实现优雅退出。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

终端 1（服务器）：

```bash
./build/chat_server 127.0.0.1 9300
```

终端 2/3（多个客户端，用户名不同）：

```bash
./build/chat_client 127.0.0.1 9300 alice
./build/chat_client 127.0.0.1 9300 bob
```

- 每个客户端会收到加入/离开系统通知与所有人的消息
- 输入 `/quit` 或 Ctrl+D 退出；服务器断开时客户端自动退出

## 测试

```bash
ctest --test-dir build --output-on-failure
```

`test_chat` 为三客户端集成测试：验证同房间广播与回显、跨房间消息隔离、`/rooms` 输出、切换房间与离开通知（端口由内核分配，无端口冲突）。

## CI/CD

本地一键构建 + 测试（与 CI 相同流程，任何环境可用）：

```bash
./scripts/ci_local.sh [构建目录]
```

CI/CD 由 GitHub Actions（`.github/workflows/ci.yml`）驱动：

- **CI**：push / PR 触发，gcc 与 clang 双编译器构建，并跑 ASan+UBSan 构建；均执行 `ctest`
- **CD**：推送 `v*` 标签触发，构建 Release 后打包 `chat_server` / `chat_client` 为 `tcp_chat_room-<tag>-linux-amd64.tar.gz`，自动创建 GitHub Release 并上传
