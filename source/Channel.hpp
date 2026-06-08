#pragma once
#include <iostream>
#include <cstdint>
#include <functional>
#include <sys/epoll.h>
#include<unistd.h>
//class Poller;
class EventLoop;
class Channel
{
private:
    int _fd;
   // Poller* _poller;
    EventLoop* _eventloop;
    uint32_t _event;  // 表示需要监控的事件
    uint32_t _revent; // 表示实际触发的事件
    using eventcallback_t = std::function<void()>;
    eventcallback_t _Read_Callback;  // 可读事件回调
    eventcallback_t _Write_Callback; // 可写事件回调
    eventcallback_t _close_Callback; // 挂断事件回调(此处应该是RDHUO半连接挂断)
    eventcallback_t _Err_Callback;   // 错误事件回调
    eventcallback_t _Event_Callback; // 任意事件回调
public:
    Channel() =default;
   // Channel(int fd,Poller* poller):_fd(fd),_poller(poller){}
    Channel(int fd,EventLoop* EventLoop):_fd(fd),_eventloop(EventLoop){}
    ~Channel(){
         close(_fd);
    }
    int Get_Fd()
    {
        return _fd;
    }
    uint32_t Get_Event()
    {
        return _event;
    }
    bool Fd_Is_Read()
    {
        return _event & EPOLLIN;
        
    } // 文件描述符是否可读
    bool Fd_Is_Write()
    {
        return _event & EPOLLOUT;
    }
    void Set_Revent(uint32_t event)
    {
        // 设置已经触发的事件
        _revent = event;
    }
    // 此处往下的到回调前的所有函数后续需要添加Eventloop当中的回到
    // 添加或者解除事件监控
    // 文件描述符是否可写
    void Fd_Add_Write()
    {
        _event |= EPOLLOUT;
        //_event 已经被修改了 ,另外一段能感知到
        unpate();
    } // 对文件描述符添加可读
    void Fd_Add_Read()
    {
        _event |= EPOLLIN;
        unpate();
    } // 对文件描述添加可写
    void Fd_Delete_Write()
    {
        _event &= (~EPOLLOUT);
        unpate();
    } // 解除该文件描述符的可写事件
    void Fd_Delete_Read()
    {
        _event &= (~EPOLLIN);
        unpate();
    } // 解除该文件描述符可读事件的监控
    void Fd_Delete_All_Event()
    {
        _event = 0;
        unpate();
    } // 解除该文件描述符所有事件的监控

    // 对各个事件的回调进行设置
    void Set_Read_Callback(const eventcallback_t &cb)
    {
        _Read_Callback = cb;
    }

    void Set_Write_Callback(const eventcallback_t &cb)
    {
        _Write_Callback = cb;
    }

    void Set_close_Callback(const eventcallback_t &cb)
    {
        _close_Callback = cb;
    }

    void Set_Err_Callback(const eventcallback_t &cb)
    {
        _Err_Callback = cb;
    }

    void Set_Event_Callback(const eventcallback_t &cb)
    {
        _Event_Callback = cb;
    }
    void unpate();
    void Remove(); // 移除对该文件描述符的监控
    void HanderEvent()
    {
        if ((_revent & EPOLLIN) || (_revent & EPOLLRDHUP) || (_revent & EPOLLPRI))
        {
            // 对于后面两个标志位
            // RDHUP表示对方已经发送了FIN,关闭了写端,处于半连接状态,我们需要处理剩下的数据
            // 下一个是我们这边PRI表示优先级数据,我们需要进行处理
             if (_Event_Callback)
                _Event_Callback();
            if (_Read_Callback)
            {
                _Read_Callback();
            }
            // if (_Event_Callback)
            //     _Event_Callback();
        }
        //对于可能断开连接的操作,我们一次只执行一个
        if (_revent & EPOLLOUT)
        {
             if (_Event_Callback)
                _Event_Callback();
            if (_Write_Callback)
            {
                _Write_Callback();
            }
            // if (_Event_Callback)//刷新活跃度,如果我们放到前面可能发送数据结束下一轮就会走到释放分支了
            //     _Event_Callback();
        }
        else if (_revent & EPOLLHUP)
        {
            if (_Event_Callback)
                _Event_Callback();//此处必须放到释放之前刷新活跃度,释放之后刷新会直接崩溃
            if (_close_Callback)
            {
                _close_Callback();
            }
        }
        else if (_revent & EPOLLERR)
        {
            if (_Event_Callback)
                _Event_Callback();
            if (_Err_Callback)
            {
                _Err_Callback();
            }
        }

    } // 对事件的执行函数
};