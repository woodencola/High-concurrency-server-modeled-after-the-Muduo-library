#pragma once
#include<iostream>
#include<thread>
#include<mutex>
#include<functional>
#include<fcntl.h>
#include<cstdint>
#include<unistd.h>
#include<vector>
#include"Channel.hpp"
#include"Poller.hpp"


class EventLoop
{
    
    private:
    using Func  = std::function<void()>;
    uint32_t _eventfd;//用来当没有事件处理的时候退出epollwait的等待或者后面和timeid做定时器的
    std::vector<Func> _task_queue;//不采用队列,后面直接拷贝,提高效率
    std::mutex _mutex;//锁任务队列,保证线程安全
    std::thread::id _thread_id;//判断当前任务是否处于同一个线程下
    Poller* _poller;
    public:
    void RunAlltask()
    {

    }
    public:
    void RunInLoop(Channel* ch)
    {
        //当前线程要是处于和eventloop一个,就直接执行,否则减价到任务队列
    }
    void QueueInLoop(Channel* ch)
    {
        //把一个任务添加到任务队列当中发
    }
    bool ThreadInLoop()
    {
        //判断当前线程是否处于和_eventloop一个线程的

    }
    void UpdateEvent(Channel* ch)//添加或者修改一个fd or channel
    {

    }
    void RemoveEvent(Channel* ch)//移除 channel
    {
        
    }
};