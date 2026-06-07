#include<iostream>
#include<string>
#include<unistd.h>
#include"../source/Socket.hpp"

int main()
{
    Socket client;
    client.CreateClientConnect(1314,"127.0.0.1");
    std::string sendmsg = "mutou666";
    while (1)
    {
        client.Send(sendmsg.c_str(),sendmsg.size());
    char buffer[1024] = {0};
    client.Recv(buffer,sizeof(buffer)-1);
    std::cout<<buffer<<std::endl;
    sleep(1);
    }
    
    return 0;
}