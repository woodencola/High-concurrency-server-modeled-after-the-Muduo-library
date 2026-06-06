#pragma once
#include<iostream>
#include<cstdint>
#include<functional>
#include<sys/epoll.h>
class Channel
{
    private:
    uint32_t _event;//表示需要监控的事件
    uint32_t _revent; //表示实际触发的事件
    using eventcallback_t  = std::function<void()>;
    eventcallback_t _Read_Callback; //可读事件回调
    eventcallback_t _Write_Callback;//可写事件回调
    eventcallback_t _close_Callback;//挂断事件回调(此处应该是RDHUO半连接挂断)
    eventcallback_t _Err_Callback;//错误事件回调
    eventcallback_t _Event_Callback;//任意事件回调
    public:
    bool Fd_Is_Read(){
        return _event&EPOLLIN;
    }//文件描述符是否可读
    bool Fd_Is_Write(){
        return _event&EPOLLOUT;
    }
    void Set_Revent(uint32_t event)
    {
        //设置已经触发的事件
        _revent = event;
    }
    //此处往下的到回调前的所有函数后续需要添加Eventloop当中的回到
    //添加或者解除事件监控
    //文件描述符是否可写
    void Fd_Add_Write(){
        _event|=EPOLLOUT;
    } //对文件描述符添加可读
    void Fd_Add_Read(){
        _event|=EPOLLIN;
    }//对文件描述添加可写
    void Fd_Delete_Write(){
        _event&=(~EPOLLOUT);
    } //解除该文件描述符的可写事件
    void Fd_Delete_Read(){
        _event&=(~EPOLLIN);
    } // 解除该文件描述符可读事件的监控
    void Fd_Delete_All_Event(){
        _event = 0;
    } //解除该文件描述符所有事件的监控


    //对各个事件的回调进行设置
    void Set_Read_Callback(const eventcallback_t& cb){
        _Read_Callback = cb;
    }

    void Set_Write_Callback(const eventcallback_t& cb){
        _Write_Callback = cb;
    }

    void Set_close_Callback(const eventcallback_t& cb){
        _close_Callback = cb;
    }

    void Set_Err_Callback(const eventcallback_t& cb){
        _Err_Callback = cb;
    }
    
    void Set_Event_Callback(const eventcallback_t& cb){
        _Event_Callback = cb;
    }
    void Remove(){}//移除对该文件描述符的监控
    void HanderEvent(){
        if((_revent&EPOLLIN)||(_revent&EPOLLRDHUP)||(_revent&EPOLLPRI))
        {
            //对于后面两个标志位
            //RDHUP表示对方已经发送了FIN,关闭了写端,处于半连接状态,我们需要处理剩下的数据
            //下一个是我们这边PRI表示优先级数据,我们需要进行处理
            if(_Read_Callback)
            {
                _Read_Callback();
            }
        }
        else if(_revent&EPOLLOUT)
        {
            if(_Write_Callback)
            {
                _Write_Callback();
            }
        }
        else if(_revent&EPOLLHUP)
        {
            if(_close_Callback)
            {
                _close_Callback();
            }
        }
        else if(_revent&EPOLLERR)
        {
            if(_Err_Callback)
            {
                _Err_Callback();
            }
        }
        if(_Event_Callback)
         _Event_Callback();
    }//对事件的执行函数
};