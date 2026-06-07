#include<iostream>
#include<string>
#include<functional>
#include"../source/Socket.hpp"
#include"../source/Poller.hpp"
#include"../source/Channel.hpp"
#include"../source/Log.hpp"
void Close(Channel* ch)
{
    std::cout<< "close"<<ch->Get_Fd()<<std::endl;
    ch->Remove();
    delete ch;
}
void Read(Channel* ch)
{
    int fd = ch->Get_Fd();
    char buffer[1024] = {0};
    ssize_t  ret = recv(fd,buffer,1023,0);
    if(ret<=0)
    {
        Close(ch);
        return;
    }
    ch->Fd_Add_Write();
    std::cout<<buffer<<std::endl;

}

void Write(Channel* ch)
{
    std::string sendmsg = "我爱你";
    ssize_t ret = send(ch->Get_Fd(),sendmsg.c_str(),sendmsg.size(),0);
    if(ret<0)
    {
        ERR_LOG("send error");
        Close(ch);
        return;
    }
    ch->Fd_Delete_Write();


} 
void ERR_callbakc(Channel* ch)
{
    Close(ch);
}
void Eevent(Channel* ch)
{
    std::cout<<"i have a plan"<<std::endl;
}
void Accpet(Poller* _poller,Channel* ch)
{
    int fd = ch->Get_Fd();
    int newfd = accept(fd,nullptr,nullptr);
    if(newfd<0)
    {
        ERR_LOG("not get newfd");
        return;
    }
    Channel* io =  new Channel(newfd,_poller);
    io->Set_close_Callback(std::bind(Close,io));
    io->Set_Err_Callback(std::bind(ERR_callbakc,io));
    io->Set_Event_Callback(std::bind(Eevent,io));
    io->Set_Read_Callback(std::bind(Read,io));
    io->Set_Write_Callback(std::bind(Write,io));
    io->Fd_Add_Read();

}
int main()
{
    Poller p;
    Socket Server;
    Server.CreateServerConnect(1314);
    int fd = Server.Get_fd();
    Channel* lis_ser = new Channel(fd,&p);
    lis_ser->Set_Read_Callback(std::bind(Accpet,&p,lis_ser));
    lis_ser->Fd_Add_Read();
    while(1)
    {
        std::vector<Channel*> active;
        p.Poll(&active);
        for(auto& e:active)
        {
            e->HanderEvent();
        }
    }
    Server.Close();
    return 0;
}