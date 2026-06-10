#pragma once
#include "Log.hpp"
#include "Socket.hpp"
#include "Buffer.hpp"
#include "Channel.hpp"
#include "any.hpp"
#include "EventLoop.hpp"

#include <iostream>
#include <memory>
#include <functional>
using namespace Buffer_Module;
// 1.状态
// 2.是否销毁
// 3.eventloop

//,是否需要添加写事件,是由外部决定的
typedef enum
{
    DISCONNECTED, // 表示连接关闭
    CONNECTING,   // 连接创建,可以进行处理的状态
    CONNECTED,    // 连接成功,设置完成,可以通信
    DISCONNECTING // 表示连接准备关闭,仍然需要关心是否,输入缓冲区的数据释放处理,发送缓冲区的数据是否还要残留需要发送
} CONN_STATUS;
class Connection : public std::enable_shared_from_this<Connection>
{
    using ConnPtr = std::shared_ptr<Connection>;

private:
    int _Conn_Id;  // 连接的唯一id ,方面后面查找管理
    int _Timer_Id; // 定时器id,我们需要添加定时器,需要唯一标识,这里用Conn id的值即可,因为只需要保证唯一性
    int _Sockfd;
    CONN_STATUS _Status;     // 当前连接的状态
    bool Is_Enable_Time_del; // 是否启用连接超时销毁
    Socket _Socket;          // 套接字管理
    Channel _Channel;        // fd事件管理
    Buffer _Inbuffer;        // 输入缓冲区
    Buffer _Outbuffer;       // 输出缓冲区
    Any _Context;            // 协议切换,上下文数据处理
    EventLoop *_loop;        // 一个连接(iofd)需要关联一个EventLoop(subreactor), 一个EventLoop需要关联一个线程 ,这个线程有整体的
    // 线程池处理,由masterreactor控制
    using Conn_Connect_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>; // 将数据放到输出缓冲区中(inbuffer) 业务处理
    Conn_Connect_Callback _Connect_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    /*组件内的连接关闭回调-组件内设置的，因为服务器组件内会把所有的连接管理起来，⼀旦某个连接要关闭*/
    /*就应该从管理的地方移除掉自己的信息*/
    Conn_Close_Callback _server_closed_callback;
    Msg_Callback _Msg_Cb;

private:
    void HanderRead()
    {
        // 处理读事件
        char buffer[65536] = {0};

        // 从socket当中获取数据
        ssize_t ret = _Socket.RecvNoBlock(buffer, 65535);
        // 将数据放到inbuffer当中,调用读事件回调
        if (ret < 0)
        {
            ERR_LOG("connect error");
            ShutDownInLoop();
            return;
        }
        _Inbuffer.WriteAndAdd(buffer, ret);

        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
    }
    void HanderWrite()
    {
        // 处理写事件,调用写事件的回调
        ssize_t ret = _Socket.SendNoBlock(_Outbuffer.GetCurrentReadPosition(), _Outbuffer.CurrentEnableReadSpaceSize());
        if (ret < 0)
        {
            // 如果还有数据需要进行处理的话,表现在输入缓冲区还有值
            if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
            {
                if (_Msg_Cb)
                    _Msg_Cb(shared_from_this(), &_Inbuffer);
            }
            ReleaseInloop();
            return;
        }
        _Outbuffer.MoveReadPosition(ret);
        if (_Outbuffer.CurrentEnableReadSpaceSize() == 0)
        {
            // 输出缓冲区的可读数据为 0,关闭写事件,发送事件结束
            _Channel.Fd_Delete_Write();
            if (_Status == DISCONNECTING)
            {
                return ReleaseInloop();
            }
        }
        // 将数据从输出缓冲区发送
    }
    void HanderClose()
    {
        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
        ReleaseInloop();
        return;
    }
    void HanderErr()
    {
        return HanderClose();
    }
    void HanderEvent()
    {
        // 1.刷新活跃度
        if (Is_Enable_Time_del == true)
        {
            _loop->TimerFlush(_Timer_Id);
        }
        // 2.触发用户的回调
        if (_Event_Cb)
            _Event_Cb(shared_from_this());
    }

  
    void ReleaseInloop()
    {
        // 实际的释放接口
        // 判断状态是否是半关闭
        // if(_Status!=DISCONNECTING)
        // {
        //      ERR_LOG("状态不对");
        //     return;
        // }
        // 设置状态
        _Status = DISCONNECTED;
        // 关闭事件监控
        _Channel.Remove();
        // 关闭套接字
        _Socket.Close();
        // 如果有定时任务要取消,否则会出现野指针访问的问题
        if (_loop->hastimer(_Timer_Id))
            DisableTimeoutDelInLoop();

        // 触发用户的回调
        if (_Close_Cb)
            _Close_Cb(shared_from_this());
        // 触发服务器的回调,删除服务器内部的管理信息
        if (_server_closed_callback)
            _server_closed_callback(shared_from_this());
    }
    void EstablishedInLoop()
    {
        // 1. 判断状态是否为CONNECTING
        if (_Status != CONNECTING)
        {
            ERR_LOG("状态不对");
            return;
        }
        // 开启读监控
        _Channel.Fd_Add_Read();
        // 触发用户的读事件
        if (_Connect_Cb)

            _Connect_Cb(shared_from_this());
    }
    // 将数据放到输出缓冲区,不是实际的发送
    void SendInLoop(char *data, ssize_t len)
    {
        // 连接关闭无法释放
        if (_Status == DISCONNECTED)
            return;
        _Outbuffer.WriteAndAdd(data, len);
        if (_Channel.Fd_Is_Write() == false)
        {
            _Channel.Fd_Add_Write();
        }
    }
    void ShutDownInLoop()
    {
        // 设置状态
        _Status = DISCONNECTING;
        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            // 有数据处理一下
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
        if (_Outbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            // 处理号的数据发送一下
            if (_Channel.Fd_Is_Write() == false)
                _Channel.Fd_Add_Write();
        }

        //写入数据失败关闭,或者连接断开
        if (_Outbuffer.CurrentEnableReadSpaceSize() == 0)
        {
            ReleaseInloop();
        }
    }
    void EnableTimeoutDelInLoop(int sec)
    {
        //设置可销毁
        Is_Enable_Time_del = true;
        //如果存在就不设置了
        if(_loop->hastimer(_Timer_Id))
        {
            _loop->TimerFlush(_Timer_Id);
        }
        //不存在就设置一下
        _loop->TimerAdd(_Timer_Id,sec,std::bind(&Connection::ReleaseInloop,this));

        
    }
    void DisableTimeoutDelInLoop()
    {
        //设置不可以销毁
        Is_Enable_Time_del = false;
         if(_loop->hastimer(_Timer_Id))
        {
            _loop->TimeRemove(_Timer_Id);
        }
    }
    void ChangeProtocalInLoop(const Any &Context,
                              const Conn_Connect_Callback &Read_Cb,
                              const Conn_Write_Callback &Write_Cb,
                              const Conn_Close_Callback &Close_Cb,
                              const Conn_Event_Callback &Event_Cb,
                              const Msg_Callback &Msg_Cb)
                              {
                                _Context = Context;
                                _Connect_Cb = Read_Cb;
                                _Write_Cb = Write_Cb;
                                _Close_Cb = Close_Cb;
                                _Msg_Cb = Msg_Cb;
                              }

public:
    Connection(int Conn_Id, int Sockfd, EventLoop *loop)
        : _Conn_Id(Conn_Id), _Sockfd(Sockfd), _loop(loop)
    {
        _Timer_Id = _Conn_Id;
    }
    ~Connection() {}
    // 获取conn_id
    int Get_Id();
    // 获取sockfd
    int Get_Fd();
    // 判断当前连接是否处于connect状态
    bool Connected();
    // 获取上下文
    Any *Get_Context();
    // 设置上下文
    void Set_Context(const Any &context);
    // 发送数据
    void Send(char *data, ssize_t len);
    // 开启超时销毁
    void EnableTimeoutDel(int sec);
    // 关闭超时销毁
    void DisableTimeoutDel();
    void Shutdown();    // 给外部提供的关闭连接的接口,在这里并非真正上的关闭,仍然需要关心输入缓冲区的数据释放处理,发送缓冲区的数据是否还要残留需要发送
    void Established(); // 当连接建立完成,我们需要给Channel ,设置各种回调,调用connect_callback,该函数也需要放到Eventloop当中实现
    // 切换协议
    void ChangeProtocal(const Any &Context,
                        const Conn_Connect_Callback &Connect_Cb,
                        const Conn_Write_Callback &Write_Cb,
                        const Conn_Close_Callback &Close_Cb,
                        const Conn_Event_Callback &Event_Cb,
                        const Msg_Callback &Msg_Cb);

    void Set_Conn_Connect_Callback(const Conn_Connect_Callback &cb)
    {
    }
    void Set_Conn_Write_Callback(const Conn_Write_Callback &cb)
    {
    }
    void Set_Conn_Close_Callback(const Conn_Close_Callback &cb)
    {
    }
    void Set_Conn_Event_Callback(const Conn_Event_Callback &cb)
    {
    }
    void Set_Msg_Callback(const Msg_Callback &cb)
    {
    }
};