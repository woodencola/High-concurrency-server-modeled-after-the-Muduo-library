#pragma once
#include"Channel.hpp"
#include<sys/epoll.h>
#include<unordered_map>
#include<vector>


const static int epoll_event_sum = 1024;
class Poller
{
    private:
    int _epfd;
    epoll_event __event[epoll_event_sum];
    std::unordered_map<int,Channel*> _mp;
    bool Fd_Is_Exist(Channel& ch)
    {

    }
    bool Epoll_Opr(Channel& ch,int opr)
    {
        
    }

    
    public:
    //更新或者修改事件监控
    void UpdateEvent(Channel& ch){}
    //移除事件监控
    void RemoveEvent(Channel& ch){}
    //当前正在活跃的epoll事件
    void Poll(std::vector<struct epoll_event>& ret){}
    

}