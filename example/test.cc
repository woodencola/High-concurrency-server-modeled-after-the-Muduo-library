#include<unistd.h>
#include"timewheel.hpp"
using namespace  Time_Queue_Module;

class task{
    public:

    task()
    {
        std::cout<<" 构造"<<std::endl;
    }
    ~task()
    {
        std::cout<<" 析构"<<std::endl;

    }
};
void del(task* t)
{
    delete t;
}
int main()
{
   
    time_task_wheel tw;
     task* tt = new task();
    tw.set_time_task(111111,10,std::bind(del,tt));
    for(int i = 0;i<3;i++)
    {
         sleep(1);
        tw.flush_time_task(111111);
        tw.run_timer();
        std::cout<<"aaaa"<<std::endl;
       
    }
    for(;;)
    {
          tw.run_timer();
          std::cout<<"------------"<<std::endl;
          sleep(1);
    }
    return 0;
}