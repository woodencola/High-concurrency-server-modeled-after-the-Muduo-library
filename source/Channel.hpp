#pragma once
#include<iostream>
#include<cstdint>
#include<functional>
class Channel
{
    private:
    uint32_t _event;//表示需要监控的事件
    uint32_t _revent; //表示实际触发的事件
    using eventcallback_t  = std::function<void()>;
    eventcallback_t _Read_Callback; //可读事件回调
    eventcallback_t _Write_Callback;//可写事件回调
    eventcallback_t _Hup_Callback;//挂断事件回调(此处应该是RDHUO半连接挂断)
    eventcallback_t _Err_Callback;//错误事件回调
    eventcallback_t _Event_Callback;//任意事件回调
    public:
    bool Fd_Is_Read(){}//文件描述符是否可读
    bool Fd_Is_Write(){}//文件描述符是否可写
    void Fd_Add_Write(){} //对文件描述符添加可读
    void Fd_Add_Read(){}//对文件描述添加可写
    void Fd_Delete_Write(){} //解除该文件描述符的可写事件
    void Fd_Delete_Read(){} // 解除该文件描述符可读事件的监控
    void Fd_Delete_All_Event(){} //解除该文件描述符所有事件的监控


    //对各个事件的回调进行设置
    void Set_Read_Callback(const eventcallback_t& cb){
        _Read_Callback = cb;
    }

    void Set_Write_Callback(const eventcallback_t& cb){
        _Write_Callback = cb;
    }

    void Set_Hup_Callback(const eventcallback_t& cb){
        _Hup_Callback = cb;
    }

    void Set_Err_Callback(const eventcallback_t& cb){
        _Err_Callback = cb;
    }
    
    void Set_Event_Callback(const eventcallback_t& cb){
        _Event_Callback = cb;
    }
};