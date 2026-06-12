#include "../Sever.hpp"

class EchoServer
{
private:
    TcpSever _tcp_Server;
    void Connect_hander(const ConnPtr &_con, Buffer *_buffer)
    {
        // DBG_LOG("%s", _buffer->GetCurrentReadPosition());
        //_buffer->MoveReadPosition(_buffer->CurrentEnableReadSpaceSize());
    //   std::string msg = "我爱你111";
     _buffer->MoveReadPosition(_buffer->CurrentEnableReadSpaceSize());
      std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
        _con->Send(response.c_str(),response.size());
       
        //  _con->Shutdown();
    }

    void CLose_hander(const ConnPtr &_con)
    {
        DBG_LOG("client %p", _con.get());
    }
    void ON_hander(const ConnPtr &_con)
    {
        DBG_LOG("new connection %p", _con.get());
    }
    void Eevent(const ConnPtr &_con)
    {
        // std::cout<<"i have a plan"<<std::endl;
        //ERR_LOG("I Have a plan");
    }
     void OnConnected(const ConnPtr &conn) {
            DBG_LOG("NEW CONNECTION:%p", conn.get());
        }
        void OnClosed(const ConnPtr &conn) {
            DBG_LOG("CLOSE CONNECTION:%p", conn.get());
        }
        void OnMessage(const ConnPtr &conn, Buffer *buf) {
            conn->Send(buf->GetCurrentReadPosition(), buf->CurrentEnableReadSpaceSize());
            buf->MoveReadPosition(buf->CurrentEnableReadSpaceSize());
            conn->Shutdown();
        }
public:
    EchoServer(uint16_t port) : _tcp_Server(port)
    {
        _tcp_Server.Set_Msg_Callback(std::bind(&EchoServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
       // _tcp_Server.Set_Conn_Event_Callback(std::bind(&EchoServer::Eevent, this, std::placeholders::_1));
        _tcp_Server.Set_Conn_Close_Callback(std::bind(&EchoServer::OnClosed, this, std::placeholders::_1));
        _tcp_Server.Set_Conn_Connect_Callback(std::bind(&EchoServer::OnConnected, this, std::placeholders::_1));
        _tcp_Server.Enable_Is_Delay_del(10);
        _tcp_Server.Set_Slave_Thread_Cnt(8);
    }
    void Start()
    {
        _tcp_Server.Start();
    }
};