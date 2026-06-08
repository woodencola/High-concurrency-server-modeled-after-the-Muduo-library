#include<iostream>
#include<string>
#include<functional>
#include"../source/EventLoop.hpp"
#include"../source/Socket.hpp"
#include"../source/Poller.hpp"
#include"../source/Channel.hpp"
#include"../source/Log.hpp"
void Close(Channel* ch,EventLoop* _loop)
{
    // std::cout<< "close"<<ch->Get_Fd()<<std::endl;
    ERR_LOG("close fd is %d",ch->Get_Fd());
    _loop->TimeRemove(ch->Get_Fd());
    ch->Remove();
    delete ch;
}
void Read(Channel* ch,EventLoop* _loop)
{
    int fd = ch->Get_Fd();
    char buffer[1024] = {0};
    ssize_t  ret = recv(fd,buffer,1023,0);
    if(ret<=0)
    {
        Close(ch,_loop);
        return;
    }
    ch->Fd_Add_Write();
    // std::cout<<buffer<<std::endl;
     ERR_LOG("%s",buffer);
    //ERR_LOG(buffer);

}

void Write(Channel* ch,EventLoop* _loop)
{
    std::string sendmsg = "我爱你";
    ssize_t ret = send(ch->Get_Fd(),sendmsg.c_str(),sendmsg.size(),0);
    if(ret<0)
    {
        ERR_LOG("send error");
        Close(ch,_loop);
        return;
    }
    ch->Fd_Delete_Write();


} 
void ERR_callbakc(Channel* ch,EventLoop* _loop)
{
    Close(ch,_loop);
}
void Eevent(Channel* ch,EventLoop* _loop)
{
    // std::cout<<"i have a plan"<<std::endl;
    ERR_LOG("I Have a plan");
    _loop->TimerFlush(ch->Get_Fd());
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
    Channel* io =  new Channel(newfd,_poller);
    io->Set_close_Callback(std::bind(Close,io,_poller));
    io->Set_Err_Callback(std::bind(ERR_callbakc,io,_poller));
    io->Set_Event_Callback(std::bind(Eevent,io,_poller));
    io->Set_Read_Callback(std::bind(Read,io,_poller));
    io->Set_Write_Callback(std::bind(Write,io,_poller));
    _poller->TimerAdd(newfd,10,std::bind(Close,io,_poller));
    io->Fd_Add_Read();

}
int main()
{
    //下面这两个东西下是一个模块的
     //EventLoop loop;
      auto loop = std::make_shared<EventLoop>();
    Poller p;
    //从这里算是缓冲区又是一个模块的
    Socket Server;
    
    Server.CreateServerConnect(1314);
    int fd = Server.Get_fd();
    Channel* lis_ser = new Channel(fd,loop.get());
    lis_ser->Set_Read_Callback(std::bind(Accpet,loop.get(),lis_ser));
    lis_ser->Fd_Add_Read();
    while(1)
    {
        // std::vector<Channel*> active;
        // p.Poll(&active);
        // for(auto& e:active)
        // {
        //     e->HanderEvent();
        // }
        loop->Start();
    }
    Server.Close();
    return 0;
}