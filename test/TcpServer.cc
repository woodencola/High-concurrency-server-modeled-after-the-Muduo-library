#include"../source/Sever.hpp"

void Connect_hander(const ConnPtr& _con,Buffer* _buffer)
{
     DBG_LOG("%s",_buffer->GetCurrentReadPosition());
     _buffer->MoveReadPosition(_buffer->CurrentEnableReadSpaceSize());
     std::string msg = "我爱你111";
     _con->Send(msg.c_str(),msg.size());
   //  _con->Shutdown();
}

void CLose_hander(const ConnPtr&_con)
{
   DBG_LOG("client %p",_con.get());
}
void ON_hander(const ConnPtr& _con)
{
    DBG_LOG("new connection %p",_con.get());
}
void Eevent(const ConnPtr& _con)
{
    // std::cout<<"i have a plan"<<std::endl;
    ERR_LOG("I Have a plan");
   
}
int main()
{
    TcpSever tcpserver(1314);
    tcpserver.Set_Msg_Callback(Connect_hander);
    tcpserver.Set_Conn_Event_Callback(Eevent);
    tcpserver.Set_Conn_Close_Callback(CLose_hander);
    tcpserver.Set_Conn_Connect_Callback(ON_hander);
 
    tcpserver.Enable_Is_Delay_del(10);
    tcpserver.Set_Slave_Thread_Cnt(4);
    //tcpserver.RunAfter(std::bind);
    tcpserver.Start();
    return 0;
}