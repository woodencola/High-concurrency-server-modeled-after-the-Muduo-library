# 基于Reactor模式的C++多线程TCP网络库

## 概述

本项目实现了一个轻量级、高性能的C++ TCP网络库，基于**Reactor**事件驱动模型，支持**多线程IO处理**，内置时间轮定时器、缓冲区管理、连接生命周期控制等特性。适用于开发自定义协议的高并发服务器。

### 核心特性

- **单线程EventLoop + 多线程Worker**：主线程负责accept，IO事件分配到线程池中的多个EventLoop
- **非阻塞IO + epoll**：高效的边缘/水平触发事件管理
- **时间轮定时器**：O(1)的定时任务管理，支持连接超时检测
- **用户态缓冲区**：每个连接独立输入/输出缓冲，自动扩容
- **连接生命周期安全**：使用`shared_ptr`管理连接，支持超时自动释放
- **跨平台**：基于Linux系统调用（epoll、timerfd、eventfd）

---

## 编译与依赖

- **编译器**：C++17及以上（代码使用了`std::make_unique`, `std::enable_shared_from_this`等特性）
- **系统**：Linux（依赖于`sys/epoll.h`, `sys/timerfd.h`等）
- **链接**：无需额外库（使用标准C++和系统调用）

```bash
g++ -std=c++17 -pthread your_program.cpp -o server
```

---

## 快速开始

### 最小示例：回声服务器

```cpp
#include "network_lib.h"  // 假设本库代码整合为头文件

int main() {
    TcpSever server(8888);                      // 监听8888端口
    server.Set_Slave_Thread_Cnt(4);             // 设置4个工作线程
    server.Set_Msg_Callback([](const ConnPtr& conn, Buffer* buf) {
        // 回声：将收到的数据原样发回
        std::string msg = buf->RdStringAndPop(buf->CurrentEnableReadSpaceSize());
        conn->Send(msg.c_str(), msg.size());
    });
    server.Start();                             // 启动事件循环（阻塞）
    return 0;
}
```

---

## 核心模块接口文档

### 1. TcpSever —— 服务器主类

对外主要接口，封装监听、线程池、连接管理。

#### 构造函数
```cpp
TcpSever(int port);
```
- **参数**：`port` – 监听的TCP端口

#### 配置方法
```cpp
void Set_Slave_Thread_Cnt(int cnt);
```
设置工作线程数量（不含主循环线程）。默认0表示单线程（所有IO在主线程）。

```cpp
void Enable_Is_Delay_del(int time);
```
启用连接空闲超时自动关闭。`time`单位为秒，超时后连接被释放。

#### 回调设置
```cpp
void Set_Msg_Callback(const Msg_Callback& cb);
void Set_Conn_Connect_Callback(const Conn_Connect_Callback& cb);
void Set_Conn_Write_Callback(const Conn_Write_Callback& cb);
void Set_Conn_Close_Callback(const Conn_Close_Callback& cb);
void Set_Conn_Event_Callback(const Conn_Event_Callback& cb);
```
- `Msg_Callback`：收到消息时的回调（最常用）
- `Connect_Callback`：连接建立成功时调用
- `Write_Callback`：数据发送完成（写缓冲区清空）时调用
- `Close_Callback`：连接关闭时调用
- `Event_Callback`：每次事件循环中处理该连接的事件时调用（可用于更新超时等）

回调函数签名（以`Msg_Callback`为例）：
```cpp
std::function<void(const ConnPtr&, Buffer*)>
```
其中`ConnPtr`是`std::shared_ptr<Connection>`。

#### 启动与定时任务
```cpp
void Start();
```
启动主事件循环（阻塞当前线程）。

```cpp
void RunAfter(const Function& f, int delay);
```
在`delay`秒后执行任务`f`（在主循环线程中执行）。可用于实现一次性定时任务。

---

### 2. Connection —— 连接对象

表示一个客户端连接，提供数据发送、状态查询、上下文存储等。

#### 获取信息
```cpp
int Get_Id() const;
int Get_Fd() const;
bool Connected() const;
Any* Get_Context();
void Set_Context(const Any& context);
```
- `Get_Id()`：连接唯一标识符
- `Context`：用户自定义数据（通过`Any`类型存储任意类型）

#### 数据发送与关闭
```cpp
void Send(const char* data, ssize_t len);
void Shutdown();
void Release();
```
- `Send`：将数据加入输出缓冲区并开始发送（非阻塞）
- `Shutdown`：优雅关闭（待发送缓冲区数据全部发送后关闭）
- `Release`：立即关闭连接并释放资源

#### 超时控制
```cpp
void EnableTimeoutDel(int sec);
void DisableTimeoutDel();
```
启用或禁用该连接的空闲超时自动关闭（需服务器已调用`Enable_Is_Delay_del`）。

#### 动态更换协议
```cpp
void ChangeProtocal(const Any& Context, ...回调参数...);
```
运行时更换连接的消息处理回调，可用于协议升级（如HTTP升级WebSocket）。

---

### 3. Buffer —— 缓冲区

每个连接拥有输入缓冲`_Inbuffer`和输出缓冲`_Outbuffer`，用户可在消息回调中读/写。

#### 常用读操作
```cpp
uint64_t CurrentEnableReadSpaceSize();   // 可读字节数
std::string RdStringAndPop(uint64_t len);// 读取并弹出len字节
std::string GetLineAndAdd();             // 读取一行（以'\n'结尾），并移动读指针
void ReadAndPop(void* buffer, uint64_t len);
```

#### 常用写操作
```cpp
void WriteAndAdd(const void* data, uint64_t len);
void WtStringAndAdd(std::string& s);
```

#### 缓冲区管理
```cpp
void clear();                            // 重置读写指针（不释放内存）
void EnsureWriteSpaceSize(uint64_t len); // 确保至少有len空闲空间
```

---

### 4. Any —— 类型安全容器

用于存储任意类型的值，类似于`std::any`。

```cpp
Any a = 42;
int* p = a.Get<int>();          // 返回指针，类型错误返回nullptr
std::string* s = a.Get<std::string>(); // nullptr
```

---

### 5. 定时器接口（高级用法）

若需要直接在`EventLoop`中添加定时器，可通过`TcpSever::RunAfter`（在主循环执行）或获取`EventLoop`指针（不推荐暴露）。

时间轮定时器特性：
- 精度为1秒（内部timerfd周期1秒）
- 定时任务执行后自动销毁
- 支持取消：通过任务ID取消（详见`EventLoop::TimeRemove`，但通常用连接超时即可）

---

## 回调执行线程说明

- **主循环线程**（`_base_loop`）负责：
  - 接受新连接（`Acceptor`）
  - 分发新连接到工作线程
  - 执行`RunAfter`定时任务
  
- **工作线程**（`LoopThread`）负责：
  - 已建立连接的IO事件（读、写、错误）
  - 各个连接的`Msg_Callback`等回调

> **注意**：所有用户回调（如`Set_Msg_Callback`中的lambda）均在所属连接的`EventLoop`线程中执行，因此回调内部可以直接调用`Connection`的方法（非线程安全需加锁？由于每个连接固定在一个线程，无需加锁）。但避免在回调中执行耗时操作，否则会阻塞该线程上的所有连接。

---

## 完整示例：简易HTTP服务器

```cpp
#include <iostream>
#include "network_lib.h"

void onMessage(const ConnPtr& conn, Buffer* buf) {
    std::string request = buf->GetLineAndAdd();  // 读取第一行
    if (request.empty()) return;
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
    conn->Send(response.c_str(), response.size());
    conn->Shutdown();  // 短连接，发送后关闭
}

int main() {
    TcpSever server(8080);
    server.Set_Slave_Thread_Cnt(4);
    server.Set_Msg_Callback(onMessage);
    server.Start();
    return 0;
}
```

---

## 注意事项

1. **SIGPIPE信号**：库初始化时已忽略，防止向关闭的socket写入导致进程退出。
2. **文件描述符限制**：高并发时需调整系统ulimit -n。
3. **定时器精度**：基于timerfd，每秒触发一次，不适合高精度定时。
4. **线程安全**：用户不应在非连接所属线程中调用`Connection::Send`等方法，除非通过`_loop->RunInLoop`转发。库内部已保证回调线程安全性。
5. **内存管理**：`ConnPtr`为`shared_ptr`，连接被`TcpSever::_conns`持有，关闭时自动从map中移除。

---

## 性能调优建议

- 工作线程数通常设置为`CPU核心数`。
- 启用连接超时（`Enable_Is_Delay_del`）避免空闲连接占用资源。
- 消息回调中尽量使用`Buffer::RdStringAndPop`一次性取走数据，减少多次拷贝。
- 发送大数据时可分块多次调用`Send`，库会自动处理写缓冲区。

---

## 依赖与兼容性

- Linux内核≥2.6.27（支持timerfd）
- 编译器：GCC 7+ / Clang 5+
- 无第三方库依赖

---

## 已知限制与优化方向

### 当前限制

- **时间轮最大延迟为 60 秒**  
  时间轮容量固定为 60 格，因此单次定时任务的最大延迟为 60 秒。  
  适用场景：连接空闲超时（典型值 10~30 秒）、心跳检测等。  
  > 如需超过 60 秒的定时任务，可多次调用 `RunAfter` 或改用 `std::priority_queue` 实现通用定时器。

- **短连接性能较低（QPS ~1,200）**  
  当前 `Acceptor` 每次事件循环仅 `accept` 一个连接，导致短连接场景下系统调用频繁。  
  **优化方案**：改为 `while` 循环批量 `accept` 直到 `EAGAIN`，预计 QPS 可提升至 3~5k。

- **定时器精度为 1 秒**  
  基于 `timerfd` 实现，每秒触发一次，不适合毫秒级定时任务。

- **未实现优雅退出**  
  `EventLoop::Start()` 为无限循环，没有停止机制。生产环境中需添加退出标志和 `eventfd` 唤醒。

- **连接与定时器 ID 共享计数器**  
  `TcpSever` 中 `_conn_id` 同时用于连接和 `RunAfter` 定时任务，存在潜在 ID 冲突。后续将分离为 `_next_conn_id` 和 `_next_timer_id`。

- **`eventfd` 读回调未清空全部计数**  
  当前仅调用一次 `read`，若多个唤醒事件累积，可能导致 `eventfd` 持续可读。优化为循环读取直到 `EAGAIN`。

### 已修复的问题

- ✅ `Connection::DisableTimeoutDel` 中的无限递归 bug（改为调用 `DisableTimeoutDelInLoop`）

### 后续优化计划

- [ ] 短连接批量 `accept` 优化
- [ ] 分离连接 ID 与定时器 ID
- [ ] 支持优雅退出（`EventLoop::Stop()`）
- [ ] 增加大包传输吞吐量测试（1MB 报文）
- [ ] 极限连接数稳定性测试（10k+ 连接）
- [ ] 与 muduo / asio 的详细性能对比

### 如何贡献 / 反馈

欢迎通过 Issue 或 PR 提出改进建议，特别是针对上述限制的解决方案。

---

## 关于作者

本项目基于Reactor模式实现，适合学习网络编程和C++现代特性。欢迎修改和优化。
