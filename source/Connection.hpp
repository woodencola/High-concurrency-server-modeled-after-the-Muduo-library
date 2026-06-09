#pragma once
#include "Log.hpp"
#include "Socket.hpp"
#include "Buffer.hpp"
#include "Channel.hpp"
#include "any.hpp"

#include <iostream>
#include <memory>
#include <functional>
using namespace Buffer_Module;

class Connection
{
    using ConnPtr = std::shared_ptr<Connection>;

private:
    int _Conn_Id;  // 连接的唯一id ,方面后面查找管理
    int _Timer_Id; // 定时器id,我们需要添加定时器,需要唯一标识,这里用Conn id的值即可,因为只需要保证唯一性
    int _Sockfd;
    Socket _Socket;    // 套接字管理
    Buffer _Inbuffer;  // 输入缓冲区
    Buffer _Outbuffer; // 输出缓冲区
    Any _Any;          // 协议切换,上下文数据处理
    using Conn_Read_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>; // 将数据放到输出缓冲区中(inbuffer) 业务处理
    Conn_Read_Callback _Read_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    Msg_Callback _Msg_Cb;

public:
    Connection() {}
    ~Connection() {}
    void Send(char *data, ssize_t len);
    void ShutDown();
    void EnableTimeoutDel(int sec);
    void DisableTimeoutDel();
    void ChangeProtocal(const Conn_Read_Callback &Read_Cb,
                        const Conn_Write_Callback &Write_Cb,
                        const Conn_Close_Callback &Close_Cb,
                        const Conn_Event_Callback &Event_Cb,
                        const Msg_Callback &Msg_Cb);
};