#include <iostream>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <typeinfo>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

// ==================== 日志宏 ====================
#define INF 0
#define DBG 1
#define ERR 2
#define LOG_LEVEL DBG
#define LOG(LEVEL, format, ...)                                                                \
    do                                                                                         \
    {                                                                                          \
        if (LEVEL < LOG_LEVEL)                                                                 \
            break;                                                                             \
        time_t t = time(nullptr);                                                              \
        struct tm *time = localtime(&t);                                                       \
        char formattime[32] = {0};                                                             \
        strftime(formattime, 31, "%H:%M:%S", time);                                            \
        fprintf(stdout, "[thread id is %lu:%s][%s:%d]:" format "\n",                           \
                (unsigned long)pthread_self(), formattime, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0);

#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

// ==================== 前置声明 ====================
class EventLoop;
class Poller;

// ==================== Buffer 模块 ====================
namespace Buffer_Module
{
    const static uint64_t BUFFERSIZE = 1024;

    class Buffer
    {
    private:
        std::vector<char> _buffer;
        uint64_t _read_index;
        uint64_t _write_index;

    private:
        char *begin()
        {
            return &*_buffer.begin();
        }

    public:
        char *GetCurrentWritePosition()
        {
            return begin() + _write_index;
        }

        char *GetCurrentReadPosition()
        {
            return begin() + _read_index;
        }

        uint64_t GetTailIdleSpaceSize()
        {
            return _buffer.size() - _write_index;
        }

        uint64_t GetHeadIdleSpaceSize()
        {
            return _read_index;
        }

        uint64_t CurrentEnableReadSpaceSize()
        {
            return _write_index - _read_index;
        }

        void MoveWritePosition(uint64_t len)
        {
            assert(len <= GetTailIdleSpaceSize());
            _write_index += len;
        }

        void MoveReadPosition(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            _read_index += len;
        }

        void EnsureWriteSpaceSize(uint64_t len)
        {
            if (len <= GetTailIdleSpaceSize())
            {
                return;
            }
            else if (len <= GetTailIdleSpaceSize() + GetHeadIdleSpaceSize())
            {
                uint64_t enablereadsize = CurrentEnableReadSpaceSize();
                std::copy(GetCurrentReadPosition(), GetCurrentReadPosition() + enablereadsize, begin());
                _read_index = 0;
                _write_index = enablereadsize;
            }
            else
            {
                _buffer.resize(_write_index + len);
            }
        }

    private:
        void Write(const void *data, uint64_t len)
        {
            if (len == 0)
                return;
            EnsureWriteSpaceSize(len);
            const char *d = (char *)data;
            std::copy(d, d + len, GetCurrentWritePosition());
        }

        void Read(void *buffer, uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            char *b = (char *)buffer;
            std::copy(GetCurrentReadPosition(), GetCurrentReadPosition() + len, b);
        }

        void WriteAsString(std::string &s)
        {
            Write(&s[0], s.size());
        }

        void WriteAsBuffer(Buffer &b)
        {
            Write(b.GetCurrentReadPosition(), b.CurrentEnableReadSpaceSize());
        }

        std::string ReadAsString(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            std::string ret;
            ret.resize(len);
            Read(&ret[0], len);
            return ret;
        }

        char *GetCRLF()
        {
            char *ret = (char *)memchr(GetCurrentReadPosition(), '\n', CurrentEnableReadSpaceSize());
            return ret;
        }

        std::string GetLine()
        {
            char *LF = GetCRLF();
            if (LF == nullptr)
            {
                return "";
            }
            return ReadAsString(LF - GetCurrentReadPosition() + 1);
        }

    public:
        std::string GetLineAndAdd()
        {
            std::string ret = GetLine();
            MoveReadPosition(ret.size());
            return ret;
        }

        void WriteAsBufferAndAdd(Buffer &b)
        {
            WriteAsBuffer(b);
            MoveWritePosition(b.CurrentEnableReadSpaceSize());
        }

        void ReadAndPop(void *buffer, uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            Read(buffer, len);
            MoveReadPosition(len);
        }

        void WriteAndAdd(const void *data, uint64_t len)
        {
            Write(data, len);
            MoveWritePosition(len);
        }

        void WtStringAndAdd(std::string &s)
        {
            WriteAsString(s);
            MoveWritePosition(s.size());
        }

        std::string RdStringAndPop(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            std::string ret = ReadAsString(len);
            MoveReadPosition(len);
            return ret;
        }

        void clear()
        {
            _read_index = 0;
            _write_index = 0;
        }

        Buffer() : _read_index(0), _write_index(0), _buffer(BUFFERSIZE) {}
        ~Buffer() {}
    };
}

using namespace Buffer_Module;

// ==================== Any 类 ====================
class Any
{
private:
    class Holder
    {
    public:
        Holder() {}
        virtual ~Holder() {}
        virtual const std::type_info &Type() const = 0;
        virtual Holder *clone() const = 0;
    };

    template <typename T>
    class PlaceHolder : public Holder
    {
    public:
        PlaceHolder(const T &val) : _val(val) {}
        virtual ~PlaceHolder() {}
        virtual const std::type_info &Type() const override
        {
            return typeid(_val);
        }
        virtual Holder *clone() const override
        {
            return new PlaceHolder<T>(_val);
        }

    public:
        T _val;
    };

    Holder *_content;

public:
    Any() : _content(nullptr) {}
    template <typename T>
    Any(const T &val)
    {
        _content = new PlaceHolder<T>(val);
    }
    Any(const Any &other)
    {
        _content = other._content != nullptr ? other._content->clone() : nullptr;
    }
    Any &operator=(const Any &other)
    {
        Any(other).swap(*this);
        return *this;
    }
    template <typename T>
    Any &operator=(const T &val)
    {
        Any(val).swap(*this);
        return *this;
    }
    template <typename T>
    T *Get()
    {
        if (_content == nullptr)
            return nullptr;
        if (typeid(T) != _content->Type())
            return nullptr;
        auto it = dynamic_cast<PlaceHolder<T> *>(_content);
        return it ? &(it->_val) : nullptr;
    }
    ~Any()
    {
        delete _content;
    }
    Any &swap(Any &other) noexcept
    {
        std::swap(_content, other._content);
        return *this;
    }
};

// ==================== Channel 类声明 ====================
class Channel
{
private:
    int _fd;
    EventLoop *_eventloop;
    uint32_t _event;
    uint32_t _revent;
    using eventcallback_t = std::function<void()>;
    eventcallback_t _Read_Callback;
    eventcallback_t _Write_Callback;
    eventcallback_t _close_Callback;
    eventcallback_t _Err_Callback;
    eventcallback_t _Event_Callback;

public:
    Channel(int fd, EventLoop *EventLoop);
    int Get_Fd();
    uint32_t Get_Event();
    bool Fd_Is_Read();
    bool Fd_Is_Write();
    void Set_Revent(uint32_t event);
    void Fd_Add_Write();
    void Fd_Add_Read();
    void Fd_Delete_Write();
    void Fd_Delete_Read();
    void Fd_Delete_All_Event();
    void Set_Read_Callback(const eventcallback_t &cb);
    void Set_Write_Callback(const eventcallback_t &cb);
    void Set_close_Callback(const eventcallback_t &cb);
    void Set_Err_Callback(const eventcallback_t &cb);
    void Set_Event_Callback(const eventcallback_t &cb);
    void unpate();
    void Remove();
    void HanderEvent();
};

// ==================== Poller 类 ====================
const static int epoll_event_sum = 1024;
const static int rb_size = 1024;

class Poller
{
private:
    int _epfd;
    epoll_event _epoll_event[epoll_event_sum];
    std::unordered_map<int, Channel *> _channels;

    bool Fd_Is_Exist(Channel *ch)
    {
        int fd = ch->Get_Fd();
        auto it = _channels.find(fd);
        if (it == _channels.end())
        {
            return false;
        }
        return true;
    }

    void Epoll_Opr(Channel *ch, int opr)
    {
        int fd = ch->Get_Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = ch->Get_Event();
        int ret = epoll_ctl(_epfd, opr, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("fd not is control");
        }
    }

public:
    Poller()
    {
        _epfd = epoll_create(rb_size);
        if (_epfd < 0)
        {
            ERR_LOG("epoll fd not create");
            abort();
        }
    }

    void UpdateEvent(Channel *ch)
    {
        int fd = ch->Get_Fd();
        if (!Fd_Is_Exist(ch))
        {
            Epoll_Opr(ch, EPOLL_CTL_ADD);
            _channels[fd] = ch;
            return;
        }
        Epoll_Opr(ch, EPOLL_CTL_MOD);
    }

    void RemoveEvent(Channel *ch)
    {
        int fd = ch->Get_Fd();
        auto it = _channels.find(fd);
        if (it != _channels.end())
        {
            _channels.erase(fd);
        }
        Epoll_Opr(ch, EPOLL_CTL_DEL);
    }

    void Poll(std::vector<Channel *> *ret)
    {
        int retsize = epoll_wait(_epfd, _epoll_event, epoll_event_sum, -1);
        if (retsize < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            ERR_LOG("epoll not wait errno msg:%s", strerror(errno));
            abort();
        }
        for (int i = 0; i < retsize; i++)
        {
            int fd = _epoll_event[i].data.fd;
            auto it = _channels.find(fd);
            assert(it != _channels.end());
            it->second->Set_Revent(_epoll_event[i].events);
            ret->push_back(it->second);
        }
    }
};

// ==================== 时间轮相关类 ====================
using timeer_callback_t = std::function<void()>;
using release_time_task_t = std::function<void()>;

class time_task
{
private:
    uint64_t _id;
    uint64_t _time;
    timeer_callback_t _task;
    release_time_task_t _release;
    bool _canceled;

public:
    time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
        : _id(id), _time(time), _task(task), _canceled(false) {}
    ~time_task()
    {
        if (_canceled == false)
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
    int _tick;
    int _capacity;
    std::unordered_map<uint64_t, time_weak_ptr_t> _timers;
    EventLoop *_loop;
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
        t.it_value.tv_sec = 1;
        t.it_value.tv_nsec = 0;
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
            ERR_LOG("timerfd_not_read");
            abort();
        }
    }

    void OneTime()
    {
        Timerfd_Tick();
        run_timer();
    }

    void Removetimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
        {
            _timers.erase(it);
        }
    }

    void set_time_task(uint64_t id, uint64_t time, const timeer_callback_t &task);
    void flush_time_task(uint64_t id);
    void run_timer();
    void remove_time_task(uint64_t id);

public:
    time_task_wheel(EventLoop *loop);
    ~time_task_wheel() {}

    void set_time_task_loop(uint64_t id, uint64_t time, const timeer_callback_t &task);
    void flush_time_task_loop(uint64_t id);
    void remove_time_task_loop(uint64_t id);

    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }
        return true;
    }
};

// ==================== EventLoop 类 ====================
class EventLoop
{
private:
    using Func = std::function<void()>;
    std::thread::id _thread_id;
    Poller _poller;
    int _eventfd;
    std::unique_ptr<Channel> _event_channel;
    std::vector<Func> _task_queue;
    std::mutex _mutex;
    time_task_wheel _timewheel;

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
            ERR_LOG("Write is not good");
            abort();
        }
    }

public:
    EventLoop() : _thread_id(std::this_thread::get_id()),
                  _eventfd(create_event_fd()),
                  _event_channel(new Channel(_eventfd, this)),
                  _timewheel(this)
    {
        _event_channel->Set_Read_Callback(std::bind(&EventLoop::Read_Event_fd, this));
        _event_channel->Fd_Add_Read();
    }

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
    }

    void Start()
    {
        while (true)
        {
            std::vector<Channel *> active;
            _poller.Poll(&active);
            for (auto &e : active)
            {
                e->HanderEvent();
            }
            RunAlltask();
        }
    }

    void RunInLoop(const Func &cb)
    {
        if (ThreadInLoop())
        {
            return cb();
        }
        return QueueInLoop(cb);
    }

    void QueueInLoop(const Func &cb)
    {
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _task_queue.push_back(cb);
        }
        Weak_Up_fd();
    }

    bool ThreadInLoop()
    {
        return _thread_id == std::this_thread::get_id();
    }

    void AssertInloop()
    {
        assert(_thread_id == std::this_thread::get_id());
    }

    void UpdateEvent(Channel *ch)
    {
        return _poller.UpdateEvent(ch);
    }

    void RemoveEvent(Channel *ch)
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

// ==================== Channel 成员函数实现 ====================
Channel::Channel(int fd, EventLoop *EventLoop) : _fd(fd), _eventloop(EventLoop), _event(0), _revent(0) {}

int Channel::Get_Fd() { return _fd; }
uint32_t Channel::Get_Event() { return _event; }
bool Channel::Fd_Is_Read() { return _event & EPOLLIN; }
bool Channel::Fd_Is_Write() { return _event & EPOLLOUT; }
void Channel::Set_Revent(uint32_t event) { _revent = event; }
void Channel::Fd_Add_Write()
{
    _event |= EPOLLOUT;
    unpate();
}
void Channel::Fd_Add_Read()
{
    _event |= EPOLLIN;
    unpate();
}
void Channel::Fd_Delete_Write()
{
    _event &= (~EPOLLOUT);
    unpate();
}
void Channel::Fd_Delete_Read()
{
    _event &= (~EPOLLIN);
    unpate();
}
void Channel::Fd_Delete_All_Event()
{
    _event = 0;
    unpate();
}
void Channel::Set_Read_Callback(const eventcallback_t &cb) { _Read_Callback = cb; }
void Channel::Set_Write_Callback(const eventcallback_t &cb) { _Write_Callback = cb; }
void Channel::Set_close_Callback(const eventcallback_t &cb) { _close_Callback = cb; }
void Channel::Set_Err_Callback(const eventcallback_t &cb) { _Err_Callback = cb; }
void Channel::Set_Event_Callback(const eventcallback_t &cb) { _Event_Callback = cb; }
void Channel::unpate() { _eventloop->UpdateEvent(this); }
void Channel::Remove() { _eventloop->RemoveEvent(this); }
void Channel::HanderEvent()
{
    if ((_revent & EPOLLIN) || (_revent & EPOLLRDHUP) || (_revent & EPOLLPRI))
    {
        if (_Read_Callback)
        {
            _Read_Callback();
        }
    }
    if (_revent & EPOLLOUT)
    {
        if (_Write_Callback)
        {
            _Write_Callback();
        }
    }
    else if (_revent & EPOLLHUP)
    {
        if (_close_Callback)
        {
            _close_Callback();
        }
    }
    else if (_revent & EPOLLERR)
    {
        if (_Err_Callback)
        {
            _Err_Callback();
        }
    }
    if (_Event_Callback)
        _Event_Callback();
}

// ==================== 时间轮成员函数实现 ====================
time_task_wheel::time_task_wheel(EventLoop *loop) : _capacity(60), _tick(0), _timerfd(timerfd_create_self()), _loop(loop),
                                                    _timerfd_channel(nullptr)
{
    _timerfd_channel = std::make_unique<Channel>(_timerfd, loop);
    _wheels.resize(_capacity);
    _timerfd_channel->Set_Read_Callback(std::bind(&time_task_wheel::OneTime, this));
    _timerfd_channel->Fd_Add_Read();
}

void time_task_wheel::set_time_task(uint64_t id, uint64_t time, const timeer_callback_t &task)
{
    time_task_ptr_t ptr = std::make_shared<time_task>(id, time, task);
    _timers[id] = ptr;
    ptr->set_release(std::bind(&time_task_wheel::Removetimer, this, id));
    uint64_t pos = (time + _tick) % _capacity;
    _wheels[pos].push_back(ptr);
}

void time_task_wheel::flush_time_task(uint64_t id)
{
    auto it = _timers.find(id);
    if (it == _timers.end())
    {
        return;
    }
    time_task_ptr_t ptr = it->second.lock();
    int time = ptr->delay_time();
    uint64_t pos = (time + _tick) % _capacity;
    _wheels[pos].push_back(ptr);
}

void time_task_wheel::run_timer()
{
    _tick = (_tick + 1) % _capacity;
    _wheels[_tick].clear();
}

void time_task_wheel::remove_time_task(uint64_t id)
{
    auto it = _timers.find(id);
    if (it == _timers.end())
    {
        return;
    }
    auto ptr = it->second.lock();
    if (ptr)
    {
        ptr->Cancel();
    }
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

// ==================== LoopThread 和 LoopThreadPool ====================
class LoopThread
{
private:
    std::thread _thread;
    std::condition_variable _cond;
    std::mutex _mutex;
    EventLoop *_loop;

    void Thread_Entry()
    {
        EventLoop a;
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _loop = &a;
            _cond.notify_all();
        }
        _loop->Start();
    }

public:
    LoopThread() : _thread(&LoopThread::Thread_Entry, this), _loop(nullptr) {}
    EventLoop *Get_EventLoop()
    {
        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _cond.wait(guard, [&]()
                       { return _loop != nullptr; });
            loop = _loop;
        }
        return loop;
    }
    ~LoopThread() {}
};

class LoopThreadPool
{
private:
    int _Thread_Cnt;
    int _next_EventLoop;
    EventLoop *_Base_Loop;
    std::vector<EventLoop *> _Slave_Event_Loops;
    std::vector<LoopThread *> _Threads;

public:
    LoopThreadPool(EventLoop *base_loop) : _Thread_Cnt(0), _next_EventLoop(0), _Base_Loop(base_loop) {}
    EventLoop *Get_Next_EventLoop()
    {
        if (_Thread_Cnt == 0)
            return _Base_Loop;
        _next_EventLoop = (_next_EventLoop + 1) % _Thread_Cnt;
        return _Slave_Event_Loops[_next_EventLoop];
    }
    void Set_Thread_Cnt(int n)
    {
        _Thread_Cnt = n;
    }
    void Create()
    {
        if (_Thread_Cnt > 0)
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

// ==================== Socket 类 ====================
const static int Default_Backlog = 8192;

class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    Socket(int fd) { _sockfd = fd; }
    Socket(const Socket &e) = delete;
    Socket &operator=(const Socket &e) = delete;
    ~Socket() { Close(); }

    int Get_fd() { return _sockfd; }

    bool SocketCreate()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0)
        {
            ERR_LOG("socket fd not create");
            return false;
        }
        return true;
    }

    bool Bind(uint16_t port, const std::string &ip)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
        socklen_t len = sizeof(struct sockaddr_in);
        int ret = bind(_sockfd, (const sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("bind not finish");
            return false;
        }
        return true;
    }

    bool Listen(int backlog = Default_Backlog)
    {
        int ret = listen(_sockfd, backlog);
        if (ret < 0)
        {
            ERR_LOG("listen not finish");
            return false;
        }
        return true;
    }

    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
        socklen_t len = sizeof(addr);
        int ret = connect(_sockfd, (const sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("connect not finish");
            return false;
        }
        return true;
    }

    int Accept()
    {
        int fd = accept(_sockfd, nullptr, nullptr);
        if (fd < 0)
        {
            ERR_LOG("sever not accpet fd");
            return -1;
        }
        return fd;
    }

    ssize_t Recv(void *buffer, uint64_t len, int flag = 0)
    {
        ssize_t ret = recv(_sockfd, buffer, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            ERR_LOG("recv not finish");
            return -1;
        }
        return ret;
    }

    ssize_t RecvNoBlock(void *buffer, uint64_t len)
    {
        return Recv(buffer, len, MSG_DONTWAIT);
    }

    ssize_t Send(const void *data, uint64_t len, int flag = 0)
    {
        ssize_t ret = send(_sockfd, data, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            ERR_LOG("send not finish");
            return -1;
        }
        return ret;
    }

    ssize_t SendNoBlock(void *data, uint64_t len)
    {
        return Send(data, len, MSG_DONTWAIT);
    }

    void Close()
    {
        if (_sockfd == -1)
        {
            return;
        }
        close(_sockfd);
        _sockfd = -1;
    }

    bool CreateServerConnect(uint16_t port, const std::string &ip = "0.0.0.0", bool is_block = false)
    {
        if (SocketCreate() == false)
        {
            return false;
        }
        if (is_block)
            SetNoBlock();
        SetAddressReuse();
        if (Bind(port, ip) == false)
        {
            return false;
        }
        if (Listen() == false)
        {
            return false;
        }
        return true;
    }

    bool CreateClientConnect(uint16_t port, const std::string &ip)
    {
        if (SocketCreate() == false)
        {
            return false;
        }
        if (Connect(ip, port) == false)
        {
            return false;
        }
        return true;
    }

    void SetAddressReuse()
    {
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
    }

    void SetNoBlock()
    {
        int flag = fcntl(_sockfd, F_GETFL, 0);
        fcntl(_sockfd, F_SETFL, flag | O_NONBLOCK);
    }
};

// ==================== Acceptor 类 ====================
class Acceptor
{
private:
    Socket _socket;
    Channel _channel;
    EventLoop *_loop;
    using Acceptor_Callback_t = std::function<void(int)>;
    Acceptor_Callback_t _Acceptor_cb;

    int CreaterServer(uint16_t port)
    {
        int ret = _socket.CreateServerConnect(port);
        assert(ret == true);
        return _socket.Get_fd();
    }

    void HanderRead()
    {
        int newfd = _socket.Accept();
        if (newfd < 0)
        {
            return;
        }
        if (_Acceptor_cb)
            _Acceptor_cb(newfd);
    }

public:
    Acceptor(EventLoop *loop, uint16_t port) : _socket(CreaterServer(port)), _loop(loop), _channel(_socket.Get_fd(), loop)
    {
        _channel.Set_Read_Callback(std::bind(&Acceptor::HanderRead, this));
    }
    ~Acceptor() {}
    void Set_Acceptor_Callback(const Acceptor_Callback_t &cb)
    {
        _Acceptor_cb = cb;
    }
    void listen()
    {
        _channel.Fd_Add_Read();
    }
};

// ==================== Connection 类 ====================
typedef enum
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
} CONN_STATUS;

class Connection;
using ConnPtr = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    int _Conn_Id;
    int _Timer_Id;
    int _Sockfd;
    CONN_STATUS _Status;
    bool Is_Enable_Time_del;
    Socket _Socket;
    Channel _Channel;
    Buffer _Inbuffer;
    Buffer _Outbuffer;
    Any _Context;
    EventLoop *_loop;

    using Conn_Connect_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>;

    Conn_Connect_Callback _Connect_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    Conn_Close_Callback _server_closed_callback;
    Msg_Callback _Msg_Cb;

private:
    void HanderRead()
    {
        char buffer[65536] = {0};
        ssize_t ret = _Socket.RecvNoBlock(buffer, 65535);
        if (ret < 0)
        {
            ShutDownInLoop();
            return;
        }
        _Inbuffer.WriteAndAdd(buffer, ret);
        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
    }

    void HanderWrite()
    {
        ssize_t ret = _Socket.SendNoBlock(_Outbuffer.GetCurrentReadPosition(), _Outbuffer.CurrentEnableReadSpaceSize());
        if (ret < 0)
        {
            if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
            {
                if (_Msg_Cb)
                    _Msg_Cb(shared_from_this(), &_Inbuffer);
            }
            Release();
            return;
        }
        _Outbuffer.MoveReadPosition(ret);
        if (_Outbuffer.CurrentEnableReadSpaceSize() == 0)
        {
            _Channel.Fd_Delete_Write();
            if (_Status == DISCONNECTING)
            {
                return Release();
            }
        }
    }

    void HanderClose()
    {
        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
        Release();
    }

    void HanderErr()
    {
        return HanderClose();
    }

    void HanderEvent()
    {
        if (Is_Enable_Time_del == true)
        {
            _loop->TimerFlush(_Timer_Id);
        }
        if (_Event_Cb)
            _Event_Cb(shared_from_this());
    }

    void ReleaseInloop()
    {
        _Status = DISCONNECTED;
        _Channel.Remove();
        _Socket.Close();
        if (_loop->hastimer(_Timer_Id))
            DisableTimeoutDelInLoop();
        if (_Close_Cb)
            _Close_Cb(shared_from_this());
        if (_server_closed_callback)
            _server_closed_callback(shared_from_this());
    }

    void EstablishedInLoop()
    {
        if (_Status != CONNECTING)
        {
            ERR_LOG("状态不对");
            return;
        }
        _Channel.Fd_Add_Read();
        if (_Connect_Cb)
            _Connect_Cb(shared_from_this());
    }

    void SendInLoop(Buffer &_b)
    {
        if (_Status == DISCONNECTED)
            return;
        _Outbuffer.WriteAsBufferAndAdd(_b);
        if (_Channel.Fd_Is_Write() == false)
        {
            _Channel.Fd_Add_Write();
        }
    }

    void ShutDownInLoop()
    {
        _Status = DISCONNECTING;
        if (_Inbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Msg_Cb)
                _Msg_Cb(shared_from_this(), &_Inbuffer);
        }
        if (_Outbuffer.CurrentEnableReadSpaceSize() > 0)
        {
            if (_Channel.Fd_Is_Write() == false)
                _Channel.Fd_Add_Write();
        }
        if (_Outbuffer.CurrentEnableReadSpaceSize() == 0)
        {
            Release();
        }
    }

    void EnableTimeoutDelInLoop(int sec)
    {
        Is_Enable_Time_del = true;
        if (_loop->hastimer(_Timer_Id))
        {
            _loop->TimerFlush(_Timer_Id);
        }
        // _loop->TimerAdd(_Timer_Id, sec, std::bind(&Connection::Release, this));
        _loop->TimerAdd(_Timer_Id, sec, [conn = shared_from_this()]()
                        { conn->Release(); });
    }
    // void EnableTimeoutDelInLoop(int sec) {
    //     Is_Enable_Time_del = true;
    //     if (_loop->hastimer(_Timer_Id)) {
    //         _loop->TimerFlush(_Timer_Id);
    //     }
    //     // 关键：捕获 shared_from_this，延迟到任务队列执行 Release
    //     _loop->TimerAdd(_Timer_Id, sec, [conn = shared_from_this()]() {
    //         conn->Release();
    //     });
    // }
    void DisableTimeoutDelInLoop()
    {
        Is_Enable_Time_del = false;
        if (_loop->hastimer(_Timer_Id))
        {
            _loop->TimeRemove(_Timer_Id);
        }
    }

    void ChangeProtocalInLoop(const Any &Context,
                              const Conn_Connect_Callback &Read_Cb,
                              const Conn_Write_Callback &Write_Cb,
                              const Conn_Close_Callback &Close_Cb,
                              const Conn_Event_Callback &Event_Cb,
                              const Msg_Callback &Msg_Cb)
    {
        _Context = Context;
        _Connect_Cb = Read_Cb;
        _Write_Cb = Write_Cb;
        _Close_Cb = Close_Cb;
        _Msg_Cb = Msg_Cb;
    }

public:
    Connection(int Conn_Id, int Sockfd, EventLoop *loop)
        : _Conn_Id(Conn_Id), _Sockfd(Sockfd), _loop(loop), Is_Enable_Time_del(false),
          _Socket(Sockfd), _Channel(_Sockfd, loop), _Status(CONNECTING)
    {
        _Timer_Id = _Conn_Id;
        _Channel.Set_close_Callback(std::bind(&Connection::HanderClose, this));
        _Channel.Set_Err_Callback(std::bind(&Connection::HanderErr, this));
        _Channel.Set_Event_Callback(std::bind(&Connection::HanderEvent, this));
        _Channel.Set_Read_Callback(std::bind(&Connection::HanderRead, this));
        _Channel.Set_Write_Callback(std::bind(&Connection::HanderWrite, this));
    }

    ~Connection()
    {
        DBG_LOG("clinet down %p", this);
    }

    int Get_Id() { return _Conn_Id; }
    int Get_Fd() { return _Sockfd; }
    bool Connected() { return _Status == CONNECTED; }
    Any *Get_Context() { return &_Context; }
    void Set_Context(const Any &context) { _Context = context; }
    void Release()
    {
        _loop->QueueInLoop([conn = shared_from_this()]()
                           { conn->ReleaseInloop(); });
    }
    void Send(const char *data, ssize_t len)
    {
        Buffer _b;
        _b.WriteAndAdd(data, len);
        _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, _b));
    }

    void EnableTimeoutDel(int sec)
    {
        _loop->RunInLoop(std::bind(&Connection::EnableTimeoutDelInLoop, this, sec));
    }

    void DisableTimeoutDel()
    {
        _loop->RunInLoop(std::bind(&Connection::DisableTimeoutDelInLoop, this));
    }

    void Shutdown()
    {
        _loop->RunInLoop(std::bind(&Connection::ShutDownInLoop, this));
    }

    void Established()
    {
        _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
    }

    void ChangeProtocal(const Any &Context,
                        const Conn_Connect_Callback &Connect_Cb,
                        const Conn_Write_Callback &Write_Cb,
                        const Conn_Close_Callback &Close_Cb,
                        const Conn_Event_Callback &Event_Cb,
                        const Msg_Callback &Msg_Cb)
    {
        _loop->AssertInloop();
        _loop->RunInLoop(std::bind(&Connection::ChangeProtocalInLoop, this, Context,
                                   Connect_Cb, Write_Cb, Close_Cb, Event_Cb, Msg_Cb));
    }

    void Set_Conn_Connect_Callback(const Conn_Connect_Callback &cb) { _Connect_Cb = cb; }
    void Set_Conn_Write_Callback(const Conn_Write_Callback &cb) { _Write_Cb = cb; }
    void Set_Conn_Close_Callback(const Conn_Close_Callback &cb) { _Close_Cb = cb; }
    void Set_Conn_Event_Callback(const Conn_Event_Callback &cb) { _Event_Cb = cb; }
    void Set_Msg_Callback(const Msg_Callback &cb) { _Msg_Cb = cb; }
    void Set_Server_Callback(const Conn_Close_Callback &cb) { _server_closed_callback = cb; }
};

// ==================== TcpSever 类 ====================
class TcpSever
{
    uint16_t _port;
    int _timeout;
    bool _Is_able_delay_del;
    int _Thread_cnt;
    uint64_t _conn_id;
    EventLoop _base_loop;
    Acceptor _acceptor;
    LoopThreadPool _pool;
    std::unordered_map<uint64_t, ConnPtr> _conns;

    using Conn_Connect_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>;
    using Function = std::function<void()>;

    Conn_Connect_Callback _Connect_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    Msg_Callback _Msg_Cb;

    void RemoveConnectionInLoop(const ConnPtr &conn)
    {
        uint64_t id = conn->Get_Id();
        auto it = _conns.find(id);
        if (it != _conns.end())
        {
            _conns.erase(it);
        }
    }

    void RemoveConnection(const ConnPtr &conn)
    {
        _base_loop.RunInLoop(std::bind(&TcpSever::RemoveConnectionInLoop, this, conn));
    }

    void NewConnection(int fd)
    {
        _conn_id++;
        ConnPtr conn = std::make_shared<Connection>(_conn_id, fd, _pool.Get_Next_EventLoop());
        conn->Set_Msg_Callback(_Msg_Cb);
        conn->Set_Conn_Connect_Callback(_Connect_Cb);
        conn->Set_Conn_Close_Callback(_Close_Cb);
        conn->Set_Conn_Event_Callback(_Event_Cb);
        conn->Set_Server_Callback(std::bind(&TcpSever::RemoveConnection, this, std::placeholders::_1));
        if (_Is_able_delay_del)
            conn->EnableTimeoutDel(_timeout);
        conn->Established();
        _conns.insert(std::make_pair(_conn_id, conn));
    }

    void RunAfterInLoop(const Function &f, int delay)
    {
        _conn_id++;
        _base_loop.TimerAdd(_conn_id, delay, f);
    }

public:
    TcpSever(int port) : _port(port), _timeout(0), _Is_able_delay_del(false), _Thread_cnt(0),
                         _conn_id(0), _acceptor(&_base_loop, _port), _pool(&_base_loop)
    {
        _acceptor.Set_Acceptor_Callback(std::bind(&TcpSever::NewConnection, this, std::placeholders::_1));
        _acceptor.listen();
    }

    void Start()
    {
        _pool.Set_Thread_Cnt(_Thread_cnt);
        _pool.Create();
        _base_loop.Start();
    }

    void Set_Slave_Thread_Cnt(int cnt) { _Thread_cnt = cnt; }
    void RunAfter(const Function &f, int delay)
    {
        _base_loop.RunInLoop(std::bind(&TcpSever::RunAfterInLoop, this, f, delay));
    }

    void Enable_Is_Delay_del(int time)
    {
        _Is_able_delay_del = true;
        _timeout = time;
    }

    void Set_Conn_Connect_Callback(const Conn_Connect_Callback &cb) { _Connect_Cb = cb; }
    void Set_Conn_Write_Callback(const Conn_Write_Callback &cb) { _Write_Cb = cb; }
    void Set_Conn_Close_Callback(const Conn_Close_Callback &cb) { _Close_Cb = cb; }
    void Set_Conn_Event_Callback(const Conn_Event_Callback &cb) { _Event_Cb = cb; }
    void Set_Msg_Callback(const Msg_Callback &cb) { _Msg_Cb = cb; }
};

// ==================== 网络模块初始化（忽略 SIGPIPE） ====================
class NetWork
{
public:
    NetWork()
    {
        signal(SIGPIPE, SIG_IGN);
    }
};
static NetWork nw;
