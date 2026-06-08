#include<iostream>
#include<unistd.h>
#include<fcntl.h>
#include<cstdint>
#include<sys/eventfd.h>
int main()
{
    int eventfd1 = eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
    if(eventfd1<0)
    {
        std::cerr<<"aaa"<<std::endl;
    }
    uint64_t vate =1;
    write(eventfd1,&vate,sizeof vate);    
    write(eventfd1,&vate,sizeof vate);    
    write(eventfd1,&vate,sizeof vate);    
    write(eventfd1,&vate,sizeof vate);    

    uint64_t ret  = 0;
    read(eventfd1,&ret,sizeof ret);
    std::cout<<ret<<std::endl;
    return 0;
}