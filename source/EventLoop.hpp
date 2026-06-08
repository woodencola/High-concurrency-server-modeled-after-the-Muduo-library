#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>
#include <fcntl.h>
#include <cstdint>
#include <unistd.h>
#include <sys/eventfd.h>
#include <vector>
#include "Channel.hpp"
#include "Poller.hpp"

class EventLoop
{

private:
    using Func = std::function<void()>;
    std::thread::id _thread_id; // 判断当前任务是否处于同一个线程下
    Poller _poller;
    int _eventfd; // 用来当没有事件处理的时候退出epollwait的等待或者后面和timeid做定时器的
    std::unique_ptr<Channel> _event_channel;
    std::vector<Func> _task_queue; // 不采用队列,后面直接拷贝,提高效率
    std::mutex _mutex;             // 锁任务队列,保证线程安全

public:
    void RunAlltask()
    {
        std::vector<Func> _task;
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _task.swap(_task_queue);
        }
        for (auto &e : _task)
        {
            e();
        }
        return;
    }
    static int create_event_fd()
    {
        int eventretfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (eventretfd < 0)
        {
            ERR_LOG("eventfd not create");
            abort();
        }
        return eventretfd;
    }
    void Read_Event_fd()
    {
        uint64_t tmp = 0;
        int ret = read(_eventfd, &tmp, sizeof(tmp));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            ERR_LOG("Read is not good");
            abort();
        }
    }
    void Weak_Up_fd()
    {
        uint64_t tmp = 1;
        int ret = write(_eventfd, &tmp, sizeof(tmp));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            ERR_LOG("Read is not good");
            abort();
        }
    }

public:
    EventLoop() : _thread_id(std::this_thread::get_id()), _eventfd(create_event_fd()), _event_channel(new Channel(_eventfd, this))
    {
        // 设置回调,开去读事件
        _event_channel->Set_Read_Callback(std::bind(&EventLoop::Read_Event_fd, this));
        _event_channel->Fd_Add_Read();
    }
    ~EventLoop()
    {
        if (_event_channel)
        {
            _event_channel->Remove();
        }
        close(_eventfd);
    }
    void Start()
    {
        while (true)
        {
            // 1.事件监控
            std::vector<Channel *> active;
            _poller.Poll(&active);
            // 就绪事件处理
            for (auto &e : active)
            {
                e->HanderEvent();
            }
            // 处理所有事件
            RunAlltask();
        }
    }
    void RunInLoop(const Func &cb)
    {
        // 当前线程要是处于和eventloop一个,就直接执行,否则减价到任务队列
        if (ThreadInLoop())
        {
            return cb();
        }
        return QueueInLoop(cb);
    }
    void QueueInLoop(const Func &cb)
    {
        // 把一个任务添加到任务队列当中发
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _task_queue.push_back(cb);
        }
        // 此处可能出现,我的epoll当中没有任何事件就绪,从而变为阻塞了,为了解决这种情况,我们向这个eventfd当中写个数据,但我们
        // 并不关心这个数字具体多少,单纯为了触发读事件
        Weak_Up_fd();
    }
    bool ThreadInLoop()
    {
        // 判断当前线程是否处于和_eventloop一个线程的
        return _thread_id == std::this_thread::get_id();
    }
    void UpdateEvent(Channel *ch) // 添加或者修改一个fd or channel
    {
        return _poller.UpdateEvent(ch);
    }
    void RemoveEvent(Channel *ch) // 移除 channel
    {
        return _poller.RemoveEvent(ch);
    }
};
void Channel::unpate()
{
    _eventloop->UpdateEvent(this);
}
void Channel::Remove()
{
    _eventloop->RemoveEvent(this);
}