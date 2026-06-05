#pragma once

#include <unordered_map>
#include <iostream>
#include <vector>
#include <functional>
#include<unordered_map>
#include <memory>
namespace Time_Queue_Module
{
    using timeer_callback_t = std::function<void()>;
    using release_time_task_t  =std::function<void()>;
    class time_task
    {
    private:
        u_int64_t _id;
        // 延长的时间
        u_int64_t _time;
        // 执行的定时任务
        timeer_callback_t _task;
        //对应的任务执行完需要释放
        release_time_task_t _release;
    public:
        time_task(u_int64_t id, u_int64_t time, timeer_callback_t &task)
            : _id(id), _time(time), _task(task) {}
        ~time_task()
        {
            _task();
            _release();
        }
        void set_release(release_time_task_t& cb)
        {
            _release = cb;
        }
    };
    using time_task_ptr_t  = std::shared_ptr<time_task>;
    using time_weak_ptr_t  = std::weak_ptr<time_task>;
    /// aaaaa
    class time_task_wheel
    {
        private:
        std::vector<std::vector<time_task_ptr_t>> _wheels;
        int _tick;//时间轮当中的当前时间,到了该位置就要执行任务
        int _capacity;//时间轮当中的最大容量
        std::unordered_map<u_int64_t,time_weak_ptr_t> _timers;//这个东西是为了第一次添加之后更新活跃度的
        //,weakptr本身并不会占据多余的技术,但可以让后续的shared_ptr,找到对应的资源
        public:
        time_task_wheel():_capacity(60),_tick(0){}
        ~time_task_wheel();
        void set_time_task(u_int64_t id, u_int64_t time, timeer_callback_t& task);
        void flush_time_task(u_int64_t id);
    };
}