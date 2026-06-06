#include"../source/Buffer.hpp"

#include<iostream>
using namespace Buffer_Module;
int main()
{
    Buffer b;
    for(int i = 0;i<3000;i++)
    {
        std::string tmp  = "aaaaa"+std::to_string(i)+'\n';
        b.WtStringAndAdd(tmp);
    }
    while(b.CurrentEnableReadSpaceSize()>0)
    {
        std::string line = b.GetLineAndAdd();
        std::cout<<line;
    }
    // std::string t1 = "aaaa";
  
    // b.WtStringAndAdd(t1);
    //   Buffer b1;
    // b1.WriteAsBufferAndAdd(b);
    //   std::cout<<b1.CurrentEnableReadSpaceSize()<<std::endl;
    // std::string tmp1   = b1.RdStringAndPop(b1.CurrentEnableReadSpaceSize());
    // std::cout<<tmp1<<std::endl;

    // std::cout<<b.CurrentEnableReadSpaceSize()<<std::endl;
    // std::string tmp   = b.RdStringAndPop(b.CurrentEnableReadSpaceSize());
    // std::cout<<tmp<<std::endl;
    // std::cout<<b.CurrentEnableReadSpaceSize()<<std::endl;

    return 0;
}