#pragma once

#include "LoopThread.hpp"
#include <vector>

class LoopThreadPool
{
private:
    int _Thread_Cnt; // 线程数量
    int _next_EventLoop;
    EventLoop *_Base_Loop; // 主线程的事件循环
    std::vector<EventLoop *> _Slave_Event_Loops;
    std::vector<LoopThread *> _Threads;

public:
    LoopThreadPool(EventLoop *base_loop)
        : _Thread_Cnt(0),
          _next_EventLoop(0),
          _Base_Loop(base_loop)
    {
    }
    EventLoop *Get_Next_EventLoop()
    {
        if (_Thread_Cnt==0)
            return _Base_Loop;
       
            _next_EventLoop = (_next_EventLoop + 1) % _Thread_Cnt;
            return _Slave_Event_Loops[_next_EventLoop];
        
    }
    void Set_Thread_Cnt(int n)
    {
        // 设置线程数量为0,在这里,如果我们这个里只有1个线程,就直接交给_Base_Loop和主线程
        // 如果大于0,就创建n个
        _Thread_Cnt = n;
    }
    // 创建从属线程
    void Create()
    {
       if(_Thread_Cnt>0)
       {
         _Threads.resize(_Thread_Cnt);
        _Slave_Event_Loops.resize(_Thread_Cnt);
        for (int i = 0; i < _Thread_Cnt; i++)
        {

            _Threads[i] = new LoopThread();
            _Slave_Event_Loops[i] = _Threads[i]->Get_EventLoop();
        }
       }
    }
};
