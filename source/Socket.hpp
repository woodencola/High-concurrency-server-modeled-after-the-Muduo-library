#pragma once 
#include<string>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<cstdint>

const static int Default_Backlog = 32;
class Socket
{
    private:
    int fd;
    public:
    Socket(){}
    ~Socket(){}
    //创建套接字
    void SocketCreate(){}
    //绑定网络地址信息
    bool Bind(uint16_t port,const std::string& ip)
    {}
    //给监听套接字设置监听状态
    bool listen(int backlog = Default_Backlog)
    {}
    //接收新连接
    int Accpet()
    {

    }
    //接收数据
    ssize_t Recv(void* buffer,uint64_t len,int flag)
    {}
    //发送数据
    ssize_t Send(void* data,uint64_t len,int flag)
    {

    }
    //创建一个服务端连接
    bool CreateServerConnect(uint16_t port,const std::string& ip = "0.0.0.0")
    {

    }
    //创建一个客户端连接
    bool CreateClientConnect(uint16_t port,const std::string& ip)
    {

    }
    //开启地址端口重用
    void SetAddressReuse()
    {

    }
    // 设置为非阻塞
    void SetNoBlock()
    {

    }
};