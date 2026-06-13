代码 (EchoServer)
  │
  ├─ 设置 TcpSever 回调 (OnConnected, OnClosed, OnMessage, etc.)
  │
  ▼
TcpSever
  ├─ 将用户回调存储并传递给每个 Connection
  ├─ 设置 Acceptor 回调 (NewConnection)
  │
  ▼
Acceptor
  └─ 设置 Channel 读回调 → HanderRead → accept → 调用 _Acceptor_cb
      │
      ▼
  NewConnection (创建 Connection，分配 EventLoop，设置回调)
      │
      ▼
Connection
  ├─ 设置自己的 Channel 回调 (HanderRead, HanderWrite, HanderClose, ...)
  ├─ 当 Channel 事件触发时，调用对应的 HanderXxx
  │     ├─ HanderRead → 读数据 → _Msg_Cb (用户消息处理)
  │     ├─ HanderWrite → 写数据 → 可能触发 _Write_Cb (但未使用)
  │     ├─ HanderClose → Release → _Close_Cb + _server_closed_callback
  │     └─ HanderEvent → _Event_Cb + 刷新定时器
  └─ 超时定时器回调 → Release