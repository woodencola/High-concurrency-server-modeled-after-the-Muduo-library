#pragma once

#include <unordered_map>
#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
namespace Time_Queue_Module
{
    using timeer_callback_t = std::function<void()>;
    using release_time_task_t = std::function<void()>;
    class time_task
    {
    private:
        u_int64_t _id;
        // 延长的时间
        u_int64_t _time;
        // 执行的定时任务
        timeer_callback_t _task;
        // 对应的任务执行完需要释放
        release_time_task_t _release;

    public:
        time_task(u_int64_t id, u_int64_t time, const timeer_callback_t &task)
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
        u_int64_t delay_time()
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
        int _tick;                                              // 时间轮当中的当前时间,到了该位置就要执行任务
        int _capacity;                                          // 时间轮当中的最大容量
        std::unordered_map<u_int64_t, time_weak_ptr_t> _timers; // 这个东西是为了第一次添加之后更新活跃度的
        //,weakptr本身并不会占据多余的技术,但可以让后续的shared_ptr,找到对应的资源
    public:
        time_task_wheel(): _capacity(60), _tick(0) {
            _wheels.resize(_capacity);
        }
        ~time_task_wheel(){};
        void set_time_task(u_int64_t id, u_int64_t time,const  timeer_callback_t &task)
        {
            time_task_ptr_t ptr = std::make_shared<time_task>(id, time, task);
            _timers[id] = ptr;
            ptr->set_release(std::bind(&time_task_wheel::remove_time_task, this, id));
            // 找到位置延迟加上当前时间数,对整体时间取模防止越界
            u_int64_t pos = (time + _tick) % _capacity;
            _wheels[pos].push_back(ptr);
        }
        void flush_time_task(u_int64_t id)
        {
            auto it = _timers.find(id);
            if (it == _timers.end())
            {
              //  std::cerr << " not find" << std::endl;
                return;
            }
            time_task_ptr_t ptr = it->second.lock();
            int time = ptr->delay_time();
            u_int64_t pos = (time + _tick) % _capacity;
            _wheels[pos].push_back(ptr);
        }
        void run_timer()
        {
            //每一秒触发一次,由上层的定时器控制
            _tick = (_tick+1)%_capacity;
            _wheels[_tick].clear();
        }

    private:
        void remove_time_task(u_int64_t id)
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