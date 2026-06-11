#include<iostream>
#include<string>
#include<functional>
#include<vector>
#include<unordered_map>
#include "../source/Acceptor.hpp"
#include"../source/Connection.hpp"
#include"../source/EventLoop.hpp"
#include"../source/Socket.hpp"
#include"../source/Poller.hpp"
#include"../source/Channel.hpp"
#include"../source/Log.hpp"
#include"../source/LoopThread.hpp"  
#include"../source/LoopThreadPool.hpp"
LoopThreadPool* pool;
auto loop = std::make_shared<EventLoop>();

// std::vector<LoopThread> pool(2);
// int next_id = 0;
// void Close(Channel* ch,EventLoop* _loop)
// {
//     // std::cout<< "close"<<ch->Get_Fd()<<std::endl;
//     ERR_LOG("close fd is %d",ch->Get_Fd());
//     _loop->TimeRemove(ch->Get_Fd());
//     ch->Remove();
//     delete ch;
// }
// void Read(Channel* ch,EventLoop* _loop)
// {
//     int fd = ch->Get_Fd();
//     char buffer[1024] = {0};
//     ssize_t  ret = recv(fd,buffer,1023,0);
//     if(ret<=0)
//     {
//         Close(ch,_loop);
//         return;
//     }
//     ch->Fd_Add_Write();
//     // std::cout<<buffer<<std::endl;
//      ERR_LOG("%s",buffer);
//     //ERR_LOG(buffer);

// }

// void Write(Channel* ch,EventLoop* _loop)
// {
//     std::string sendmsg = "我爱你";
//     ssize_t ret = send(ch->Get_Fd(),sendmsg.c_str(),sendmsg.size(),0);
//     if(ret<0)
//     {
//         ERR_LOG("send error");
//         Close(ch,_loop);
//         return;
//     }
//     ch->Fd_Delete_Write();


// } 
// void ERR_callbakc(Channel* ch,EventLoop* _loop)
// {
//     Close(ch,_loop);
// }
// void Eevent(Channel* ch,EventLoop* _loop)
// {
//     // std::cout<<"i have a plan"<<std::endl;
//     ERR_LOG("I Have a plan");
//     _loop->TimerFlush(ch->Get_Fd());
// }
void Connect_hander(const ConnPtr& _con,Buffer* _buffer)
{
     DBG_LOG("%s",_buffer->GetCurrentReadPosition());
     _buffer->MoveReadPosition(_buffer->CurrentEnableReadSpaceSize());
     std::string msg = "我爱你111";
     _con->Send(msg.c_str(),msg.size());
    // _con->Shutdown();
}

int conn_id  =1;
std::unordered_map<int ,ConnPtr> mp;
void CLose_hander(const ConnPtr&_con)
{
    mp.erase(_con->Get_Id());
}
void ON_hander(const ConnPtr& _con)
{
    DBG_LOG("new connection %p",_con.get());
}
void Accpet(EventLoop* _poller,Channel* ch)
{
    int fd = ch->Get_Fd();
    int newfd = accept(fd,nullptr,nullptr);
    if(newfd<0)
    {
        ERR_LOG("not get newfd");
        return;
    }

    // next_id =  (next_id+1)%10;
    ConnPtr io = std::make_shared<Connection>(conn_id,newfd,_poller);
     mp[conn_id] = io;
    conn_id++;
    io->Set_Msg_Callback(std::bind(Connect_hander,std::placeholders::_1,std::placeholders::_2));
    io->Set_Conn_Connect_Callback(ON_hander);
    io->Set_Conn_Close_Callback(CLose_hander);
    io->EnableTimeoutDel(10);
    io->Established();
   
    // Channel* io =  new Channel(newfd,_poller);
    // io->Set_close_Callback(std::bind(Close,io,_poller));
    // io->Set_Err_Callback(std::bind(ERR_callbakc,io,_poller));
    // io->Set_Event_Callback(std::bind(Eevent,io,_poller));
    // io->Set_Read_Callback(std::bind(Read,io,_poller));
    // io->Set_Write_Callback(std::bind(Write,io,_poller));
    // _poller->TimerAdd(newfd,10,std::bind(Close,io,_poller));
    // io->Fd_Add_Read();
    DBG_LOG("--------------------------------------------");

}
void Accpet1(int newfd)
{
   

   
    ConnPtr io = std::make_shared<Connection>(conn_id,newfd,pool->Get_Next_EventLoop());
     mp[conn_id] = io;
    conn_id++;
    io->Set_Msg_Callback(std::bind(Connect_hander,std::placeholders::_1,std::placeholders::_2));
    io->Set_Conn_Connect_Callback(ON_hander);
    io->Set_Server_Callback(CLose_hander);
    io->EnableTimeoutDel(10);
    io->Established();
   
    // Channel* io =  new Channel(newfd,_poller);
    // io->Set_close_Callback(std::bind(Close,io,_poller));
    // io->Set_Err_Callback(std::bind(ERR_callbakc,io,_poller));
    // io->Set_Event_Callback(std::bind(Eevent,io,_poller));
    // io->Set_Read_Callback(std::bind(Read,io,_poller));
    // io->Set_Write_Callback(std::bind(Write,io,_poller));
    // _poller->TimerAdd(newfd,10,std::bind(Close,io,_poller));
    // io->Fd_Add_Read();
     DBG_LOG("--------------------------------------------");
}
int main()
{
    //下面这两个东西下是一个模块的
     //EventLoop loop;
    pool = new LoopThreadPool(loop.get());
    pool->Set_Thread_Cnt(3);
    pool->Create();
    Poller p;
    //从这里算是缓冲区又是一个模块的
    Socket Server;
    
    // Server.CreateServerConnect(1315);
    // int fd = Server.Get_fd();
    // Channel* lis_ser = new Channel(fd,loop.get());
    // lis_ser->Set_Read_Callback(std::bind(Accpet,loop.get(),lis_ser));
    // lis_ser->Fd_Add_Read();

    Acceptor a(loop.get(),1314);
    a.Set_Acceptor_Callback(std::bind(Accpet1,std::placeholders::_1));
    a.listen();
    // while(1)
    // {
    //     // std::vector<Channel*> active;
        // p.Poll(&active);
        // for(auto& e:active)
        // {
        //     e->HanderEvent();
        // }
        loop->Start();
    // }
    // Server.Close();
    return 0;
}