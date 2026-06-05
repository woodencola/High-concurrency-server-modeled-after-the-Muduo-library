#include <iostream>
#include <sys/timerfd.h>
#include <unistd.h>
#include <sys/types.h>

int main()
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
    u_int64_t timeout = 0;
    while (1)
    {
        u_int64_t time1 =0;
        int size = read(timerfd,&time1,8);
        timeout+=time1;
        std::cout<<"timeout times is"<<timeout<<std::endl;
    }
    return 0;
}