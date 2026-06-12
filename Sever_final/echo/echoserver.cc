#include "../Sever/Sever.hpp"
// ==================== EchoServer 测试类 ====================
class EchoServer
{
private:
   TcpSever _tcp_Server;

   void OnConnected(const ConnPtr &conn)
   {
      DBG_LOG("NEW CONNECTION:%p", conn.get());
   }

   void OnClosed(const ConnPtr &conn)
   {
      DBG_LOG("CLOSE CONNECTION:%p", conn.get());
   }

   //  void OnMessage(const ConnPtr& conn, Buffer* buf) {
   //    const char* response =
   //      "HTTP/1.1 200 OK\r\n"
   //      "Content-Length: 2\r\n"
   //      "Connection: close\r\n"
   //      "\r\n"
   //      "OK";

   //  // 3. 发送响应
   //  conn->Send(response, strlen(response));

   //  // 4. 消费缓冲区中的所有数据（避免下次重复触发）
   //  //    你的 Buffer 类有 MoveReadPosition，直接移动到末尾
   //  buf->MoveReadPosition(buf->CurrentEnableReadSpaceSize());

   //  // 5. 关闭连接（因为短连接，且 Connection: close）
   //  conn->Shutdown();  // 或者 conn->Shutdown() 会触发优雅关闭

   // }
   void OnMessage(const ConnPtr &conn, Buffer *buf)
   {
      const char *response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Length: 2\r\n"
          "Connection: keep-alive\r\n"
          "\r\n"
          "OK";

      conn->Send(response, strlen(response));

      buf->MoveReadPosition(
          buf->CurrentEnableReadSpaceSize());
   }

public:
   EchoServer(uint16_t port) : _tcp_Server(port)
   {
      _tcp_Server.Set_Msg_Callback(std::bind(&EchoServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
      _tcp_Server.Set_Conn_Close_Callback(std::bind(&EchoServer::OnClosed, this, std::placeholders::_1));
      _tcp_Server.Set_Conn_Connect_Callback(std::bind(&EchoServer::OnConnected, this, std::placeholders::_1));
      _tcp_Server.Enable_Is_Delay_del(10);
      _tcp_Server.Set_Slave_Thread_Cnt(2);
   }

   void Start()
   {
      _tcp_Server.Start();
   }
};

// ==================== 主函数 ====================
int main()
{
   EchoServer a(1315);
   a.Start();
   return 0;
}