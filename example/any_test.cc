#include<iostream>
#include<string>
#include<unistd.h>
#include"any.hpp"
#include<any>
using namespace std;
class task{
    public:

    task()
    {
        std::cout<<" 构造"<<std::endl;
    }
     task(const task& a)
    {
        std::cout<<" 拷贝"<<std::endl;
    }
    ~task()
    {
        std::cout<<" 析构"<<std::endl;

    }
};
int main()
{
    std::any h;
    h = 1111;
    std::cout<<*std::any_cast<int>(&h)<<std::endl;
    h = std::string("aaaaa");
    std::cout<<*std::any_cast<std::string>(&h)<<std::endl;

    // Any a(string("aaaa"));
    // {
    //     task b;
    //     a = b;
    // }
    // for(;;)sleep(1);
    // std::cout<<*a.Get<std::string>()<<std::endl;
    // a = 10;
    // std::cout<<*a.Get<int>()<<std::endl;
    // a= std::string("bbb");
    // std::cout<<*a.Get<std::string>()<<std::endl;

    return 0;
}