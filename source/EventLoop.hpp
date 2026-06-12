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
#include <sys/timerfd.h>
#include <unordered_map>
#include <time.h>
#include "Channel.hpp"
#include "Poller.hpp"

// 【主线程】MainReactor（只负责accept新连接）
//          ↓ 负载均衡分配
// ┌───────────┬───────────┬───────────┐
// SubReactor1 SubReactor2 SubReactor3 ... （EventLoop从反应堆）
// 线程1)     (线程2)     (线程3)
// 每个SubReactor 挂 N 个 Connection
using timeer_callback_t = std::function<void()>;
using release_time_task_t = std::function<void()>;
class time_task
{
private:
    uint64_t _id;
    // 延长的时间
    uint64_t _time;
    // 执行的定时任务
    timeer_callback_t _task;
    // 对应的任务执行完需要释放
    release_time_task_t _release;
     bool _canceled; 
public:
    time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
        : _id(id), _time(time), _task(task),_canceled(false) {}
    ~time_task()
    {
        if(_canceled==false)
        _task();
        _release();
    }
    void Cancel() { _canceled = true; }
    void set_release(const release_time_task_t &cb)
    {
        _release = cb;
    }
    uint64_t delay_time()
    {
        return _time;
    }
};
using time_task_ptr_t = std::shared_ptr<time_task>;
using time_weak_ptr_t = std::weak_ptr<time_task>;

class time_task_wheel
{
private:
    std::vector<std::vector<time_task_ptr_t>> _wheels;
    int _tick;                                             // 时间轮当中的当前时间,到了该位置就要执行任务
    int _capacity;                                         // 时间轮当中的最大容量
    std::unordered_map<uint64_t, time_weak_ptr_t> _timers; // 这个东西是为了第一次添加之后更新活跃度的
    //,weakptr本身并不会占据多余的技术,但可以让后续的shared_ptr,找到对应的资源
    EventLoop* _loop;
    int _timerfd;
    std::unique_ptr<Channel> _timerfd_channel;
    static int timerfd_create_self()
    {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0)
        {
            std::cerr << "timerfd not create";
            return 1;
        }

        itimerspec t;
        // 第一次的超时时间
        t.it_value.tv_sec = 1;
        t.it_value.tv_nsec = 0;
        // 超时后的时间间隔
        t.it_interval.tv_sec = 1;
        t.it_interval.tv_nsec = 0;
        timerfd_settime(timerfd, 0, &t, nullptr);
        return timerfd;
    }
    
    void Timerfd_Tick()
    {
        uint64_t tmp = 0;
        ssize_t ret = read(_timerfd, &tmp, sizeof(tmp));
        if (ret <= 0)
        {
            // if (errno == EINTR || errno == EAGAIN)
            // {
            //     return;
            // }
            ERR_LOG("timerfd_not_read");
            abort();
        }

        return;
    }
    void OneTime()
    {
        // 读取计时器里面的数据
        Timerfd_Tick();
        // 让时间轮运动
        run_timer();
    }
    void Removetimer(uint64_t id)
    {
        auto it = _timers.find(id);
            if (it != _timers.end()) {
                _timers.erase(it);
            }
    }

public:
    time_task_wheel(EventLoop* loop) : _capacity(60), _tick(0), _timerfd(timerfd_create_self()), _loop(loop),
                                                     _timerfd_channel(nullptr)
    {
        _timerfd_channel = std::make_unique<Channel>(_timerfd, loop);
        _wheels.resize(_capacity);
        _timerfd_channel->Set_Read_Callback(std::bind(&time_task_wheel::OneTime, this));
        _timerfd_channel->Fd_Add_Read();
    }
    ~time_task_wheel() {};

public:
    void set_time_task_loop(uint64_t id, uint64_t time, const timeer_callback_t &task);

    void flush_time_task_loop(uint64_t id);

    void remove_time_task_loop(uint64_t id);

    bool HasTimer(uint64_t id)
    {
        // 该函数存在线程安全问题,只能够在EventLoop线程当中使用
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            //  std::cerr << " not find" << std::endl;
            return false;
        }
        return true;
    }

private:
    void set_time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
    {
        // auto it = _timers.find(id);
        // if (it != _timers.end())
        // {
        //     auto old_ptr = it->second.lock();
        //     if (old_ptr)
        //     {
        //          old_ptr->Cancel();      
        //         RemoveTaskFromWheel(old_ptr);
        //     }
        //     _timers.erase(it);
        // }
        time_task_ptr_t ptr = std::make_shared<time_task>(id, time, task);
        _timers[id] = ptr;
        ptr->set_release(std::bind(&time_task_wheel::Removetimer, this, id));
        // 找到位置延迟加上当前时间数,对整体时间取模防止越界
        uint64_t pos = (time + _tick) % _capacity;
        _wheels[pos].push_back(ptr);
    }
    void flush_time_task(uint64_t id)
    {
         auto it = _timers.find(id);
        if (it == _timers.end())
        {
            //  std::cerr << " not find" << std::endl;
            return;
        }
        // auto ptr1 = it->second.lock();
        // if (!ptr1)
        // {
        //     // ptr->Cancel();      
        //     _timers.erase(it);
        //     return;
        // }
        // RemoveTaskFromWheel(ptr1);
        time_task_ptr_t ptr = it->second.lock();
        int time = ptr->delay_time();
        uint64_t pos = (time + _tick) % _capacity;
        _wheels[pos].push_back(ptr);
    }
    void run_timer()
    {
        // 每一秒触发一次,由上层的定时器控制
        _tick = (_tick + 1) % _capacity;
        _wheels[_tick].clear();
    }
    void remove_time_task(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            
            //  std::cerr << " not find" << std::endl;
            return;
        }
         auto ptr = it->second.lock();
        if (ptr)
        { 
             ptr->Cancel();      
            //RemoveTaskFromWheel(ptr);   
        }   
       // _timers.erase(it);
    }
};
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
    time_task_wheel   _timewheel;

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
    EventLoop() : _thread_id(std::this_thread::get_id()),
     _eventfd(create_event_fd()),
      _event_channel(new Channel(_eventfd, this)), 
      _timewheel(this)
    {
        // 设置回调,开去读事件
        //_timewheel = std::make_shared<time_task_wheel>(Get_this());
        // 这个地方不能够设置在构造函数体内，对象尚未被任何 shared_ptr 管理，shared_from_this() 抛出 std::bad_weak_ptr 异常，程序崩溃
        _event_channel->Set_Read_Callback(std::bind(&EventLoop::Read_Event_fd, this));
        _event_channel->Fd_Add_Read();
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
    void AssertInloop()
    {
         assert(_thread_id== std::this_thread::get_id());
    }
    void UpdateEvent(Channel *ch) // 添加或者修改一个fd or channel
    {
        return _poller.UpdateEvent(ch);
    }
    void RemoveEvent(Channel *ch) // 移除 channel
    {
        return _poller.RemoveEvent(ch);
    }
    void TimerAdd(uint64_t id, uint64_t time, const timeer_callback_t &task)
    {
        _timewheel.set_time_task_loop(id, time, task);
    }
    void TimerFlush(uint64_t id)
    {
        _timewheel.flush_time_task_loop(id);
    }
    void TimeRemove(uint64_t id)
    {
        _timewheel.remove_time_task_loop(id);
    }
    bool hastimer(uint64_t id)
    {
        return _timewheel.HasTimer(id);
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
void time_task_wheel::set_time_task_loop(uint64_t id, uint64_t time, const timeer_callback_t &task)
{
    _loop->RunInLoop(std::bind(&time_task_wheel::set_time_task, this, id, time, task));
}
void time_task_wheel::flush_time_task_loop(uint64_t id)
{
    _loop->RunInLoop(std::bind(&time_task_wheel::flush_time_task, this, id));
}
void time_task_wheel::remove_time_task_loop(uint64_t id)
{
    _loop->RunInLoop(std::bind(&time_task_wheel::remove_time_task, this, id));
}
