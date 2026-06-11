#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>

#include "EventLoop.hpp"

class LoopThread
{
private:
    std::thread _thread; // 线程

    // 下面这两个东西用来控制线程的内,在没去new出来时获取,_loop的一个同步与互斥

    std::condition_variable _cond; // 条件变量
    std::mutex _mutex;             // 互斥锁
    // 这个实例化对象,我们放到,线程的入口函数内部去进行实现
    // 为了防止出现,在eventloop创建到线程id分配这段真空期出现有事件触发去获取这个
    // thread_id,会被误认为在同一个地方
   std::shared_ptr< EventLoop> _loop;

    void Thread_Entry()
    {
        //这里用条件变量主要是为了防止主线程过早获取Eventloop
       auto loop = std::make_shared<EventLoop>();
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _loop = loop;
            _cond.notify_all();
        }
        _loop->Start();
    }

public:
    LoopThread() : _thread(&LoopThread::Thread_Entry, this), _loop(nullptr) {}
    EventLoop *Get_EventLoop()
    {
        // 获取当前的_loop
        EventLoop* loop = nullptr;
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _cond.wait(guard, [&]() -> bool
                       { return _loop != nullptr; });
            loop = _loop.get();
            
        }
        return loop;
    }
    ~LoopThread() {}
};