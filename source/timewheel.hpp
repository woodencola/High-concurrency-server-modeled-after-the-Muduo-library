#pragma once

#include "EventLoop.hpp"
#include "Channel.hpp"
#include <unordered_map>
#include <iostream>
#include <vector>
#include <functional>
#include <sys/timerfd.h>
#include <unordered_map>
#include <time.h>
#include <memory>
namespace Time_Queue_Module
{
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

    public:
        time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
            : _id(id), _time(time), _task(task) {}
        ~time_task()
        {
            _task();
            _release();
        }
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
    /// aaaaa
    class time_task_wheel
    {
    private:
        std::vector<std::vector<time_task_ptr_t>> _wheels;
        int _tick;                                             // 时间轮当中的当前时间,到了该位置就要执行任务
        int _capacity;                                         // 时间轮当中的最大容量
        std::unordered_map<uint64_t, time_weak_ptr_t> _timers; // 这个东西是为了第一次添加之后更新活跃度的
        //,weakptr本身并不会占据多余的技术,但可以让后续的shared_ptr,找到对应的资源
        std::weak_ptr<EventLoop> _loop;
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
            t.it_value.tv_sec = 10;
            t.it_value.tv_nsec = 0;
            // 超时后的时间间隔
            t.it_interval.tv_sec = 3;
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
                if (errno == EINTR || errno == EAGAIN)
                {
                    return;
                }
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

    public:
        time_task_wheel(std::weak_ptr<EventLoop> loop) : _capacity(60), _tick(0), _timerfd(timerfd_create_self()),
                                                         _timerfd_channel(std::make_unique<Channel>(_timerfd, _loop.lock().get()))
        {
            _wheels.resize(_capacity);
            _timerfd_channel->Set_Read_Callback(std::bind(&time_task_wheel::OneTime, this));
            _timerfd_channel->Fd_Add_Read();
        }
        ~time_task_wheel() {};

    public:
        void set_time_task_loop(uint64_t id, uint64_t time, const timeer_callback_t &task)
        {
            _loop.lock()->RunInLoop(std::bind(&time_task_wheel::set_time_task, this, id, time, task));
        }
        void flush_time_task_loop(uint64_t id)
        {
            _loop.lock()->RunInLoop(std::bind(&time_task_wheel::flush_time_task, this, id));
        }
        void remove_time_task_loop(uint64_t id)
        {
            _loop.lock()->RunInLoop(std::bind(&time_task_wheel::remove_time_task, this, id));
        }

    private:
        void set_time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
        {
            time_task_ptr_t ptr = std::make_shared<time_task>(id, time, task);
            _timers[id] = ptr;
            ptr->set_release(std::bind(&time_task_wheel::remove_time_task, this, id));
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
            _timers.erase(it);
        }
    };
}