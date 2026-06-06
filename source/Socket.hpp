#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include<cstdlib>
#include<unistd.h>
#include <errno.h>
#include<fcntl.h>
#include "Log.hpp"

const static int Default_Backlog = 32;
class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    Socket(int fd)
    {
        _sockfd = fd;
    }
    //拷贝赋值,这两个必须禁止,防止多次关闭sockfd
    Socket(const Socket& e) = delete;
    Socket& operator=(const Socket& e) = delete;
    ~Socket() {Close();}
    // 创建套接字
    bool SocketCreate()
    {
        // socket(int domian,type ,protocol)
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0)
        {
            ERR_LOG("socket fd not create");
            return false;
        }
        return true;
    }
    // 绑定网络地址信息
    bool Bind(uint16_t port, const std::string &ip)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port); // 主机转网络
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
        socklen_t len = sizeof addr;
        int ret = bind(_sockfd, (const sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("bind not finish");
            return false;
        }
        return true;
    }
    // 给监听套接字设置监听状态
    bool Listen(int backlog = Default_Backlog)
    {
        int ret = listen(_sockfd, backlog);
        if (ret < 0)
        {
            ERR_LOG("listen not finish");
            return false;
        }
        return true;
    }
    // 向服务器发起连接
    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port); // 主机转网络
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
        socklen_t len = sizeof addr;
        int ret = connect(_sockfd, (const sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("connect not finish");
            return false;
        }
        return true;
    }
    // 接收新连接
    int Accept()
    {
        int fd = accept(_sockfd, nullptr, nullptr);
        if (fd < 0)
        {
            ERR_LOG("sever not accpet fd");
            return -1;
        }
        return fd;
    }
    // 接收数据
    ssize_t Recv(void *buffer, uint64_t len, int flag = 0)
    {
        ssize_t ret = recv(_sockfd, buffer, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                // 这里是接收缓冲区没有读取到数据
                // 或者被信号打断
                // 只会存在于非阻塞版本
                return 0;
            }
            ERR_LOG("recv not finish");
            return -1;
        }
        return ret;
    }
    ssize_t RecvNoBlock(void *buffer, uint64_t len)
    {
        // MSG_DONTWAIT ,表示非阻塞
        return Recv(buffer, len, MSG_DONTWAIT);
    }
    // 发送数据
    ssize_t Send( const void *data, uint64_t len, int flag = 0)
    {
        ssize_t ret = send(_sockfd, data, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                // 这里是接收缓冲区没有读取到数据
                // 或者被信号打断
                // 只会存在于非阻塞版本
                return 0;
            }
            ERR_LOG("send not finish");
            return -1;
        }
        return ret;
    }
    ssize_t SendNoBlock(void *data, uint64_t len)
    {
        // MSG_DONTWAIT ,表示非阻塞
        return Send(data, len, MSG_DONTWAIT);
    }
    //关闭套接字
    void Close()
    {
        if(_sockfd==-1)
        {
            ERR_LOG("sockfd not exist");
            return;
        }
        close(_sockfd);
        _sockfd = -1;
    }
    // 创建一个服务端连接
    bool CreateServerConnect(uint16_t port, const std::string &ip = "0.0.0.0",bool is_)
    {
        //1.创建套接字,绑定网络信息,设置监听状态,设置非阻塞,开启地址复用
        if(SocketCreate()==false) 
        {
            return false;
        }
        if(Bind(port,ip)==false)
        {
            return false;
        }
        if(Listen()==false)
        {
            return false;   
        }
        SetNoBlock();
        SetAddressReuse();
        return true;
    }
    // 创建一个客户端连接
    bool CreateClientConnect(uint16_t port, const std::string &ip)
    {
        //1.创建套接字,2.直接向服务器发起连接
        //2.本地不绑定任何端口和ip,防止端口冲突
        if(SocketCreate()==false)
        {
            return false;
        }
        if(Connect(ip,port)==false)
        {
            return false;
        }
        return true;
    }
    // 开启地址端口重用
    void SetAddressReuse()
    {
        int val = 1;
        setsockopt(_sockfd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&val,sizeof(val));
    }
    // 设置为非阻塞
    void SetNoBlock()
    {
        int flag = fcntl(_sockfd,F_GETFD);
        fcntl(_sockfd,F_SETFD,flag|O_NONBLOCK);
    }
};