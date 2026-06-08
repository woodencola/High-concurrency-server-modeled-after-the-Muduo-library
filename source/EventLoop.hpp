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
    Channel* _event_channel;
    std::vector<Func> _task_queue;//不采用队列,后面直接拷贝,提高效率
    std::mutex _mutex;//锁任务队列,保证线程安全
    std::thread::id _thread_id;//判断当前任务是否处于同一个线程下
    Poller _poller;
    public:
    void RunAlltask()
    {
        std::vector<Func> _task;
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _task.swap(_task_queue);
        }
        for(auto& e:_task)
        {
            e();
        }
        return;
    }
    static uint32_t create_event_fd()
    {

    }
    public:
    EventLoop():_thread_id(std::this_thread::get_id()),_eventfd(create_event_fd()),_event_channel(new Channel(_eventfd,this)){}
    void Start()
    {
        //1.事件监控
        std::vector<Channel*> active;
        _poller.Poll(&active);
        // 就绪事件处理
        for(auto&e:active)
        {
            e->HanderEvent();
        }
        //处理所有事件
        RunAlltask();

    }
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