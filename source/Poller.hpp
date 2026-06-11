#pragma once
#include "Channel.hpp"
#include "Log.hpp"
#include <sys/epoll.h>
#include <unordered_map>
#include <unistd.h>
#include <vector>
#include <string.h>
#include <assert.h>

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
        // 判断描述符是否已经被管理了
        int fd = ch->Get_Fd();
        auto it = _channels.find(fd);
        if (it == _channels.end())
        {
          //  ERR_LOG("fd not is exist in _channels");
            return false;
        }
        return true;
    }
    void Epoll_Opr(Channel *ch, int opr)
    {
        // int epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event);
        int fd = ch->Get_Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = ch->Get_Event();
        int ret = epoll_ctl(_epfd, opr, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("fd not is control");
            return;
        //     if (opr == EPOLL_CTL_DEL && (errno == ENOENT || errno == EBADF)) {
        //     // 已经不存在，无需再次删除
        //     DBG_LOG("epoll_ctl DEL: fd %d already removed", fd);
        //     return;
        // }
        // // 其他错误，记录日志但不要 abort，避免压力测试崩溃
        // ERR_LOG("epoll_ctl failed, op=%d, fd=%d, errno=%d (%s)", opr, fd, errno, strerror(errno));
        // // 只有在严重错误时才 abort，例如添加新 fd 时失败
        // if (opr == EPOLL_CTL_ADD) {
        //    // abort();  // 添加失败确实严重，但可以优化为返回错误
        //    return;
        // }
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
    ~Poller()
    {
        close(_epfd);
    }
    // 更新或者修改事件监控
    void UpdateEvent(Channel *ch)
    {
        // 1.判断当前事件是否已经被管理
        int fd = ch->Get_Fd();
        if (!Fd_Is_Exist(ch))
        {
            // 不存在就添加
            Epoll_Opr(ch, EPOLL_CTL_ADD);
            _channels[fd] = ch;
            return;
        }

        Epoll_Opr(ch, EPOLL_CTL_MOD);
    }
    // 移除事件监控
    void RemoveEvent(Channel *ch)
    {
        int fd = ch->Get_Fd();
        auto it = _channels.find(fd);
        if (it != _channels.end())
        {
            _channels.erase(fd);
            Epoll_Opr(ch, EPOLL_CTL_DEL);
        }
    }
    // 当前正在活跃的epoll事件
    void Poll(std::vector<Channel *> *ret)
    {
        // int epoll_wait(int epfd, struct epoll_event *events,
        //               int maxevents, int timeout);

        // 返回值是就绪的个数,
        // timeout -1 表示阻塞
        // timeout  0 表示非阻塞
        // timeout >0 表示有超时时间
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
