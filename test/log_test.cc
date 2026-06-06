#include<iostream>
#include"../source/Log.hpp"


int main()
{
    INF_LOG("aaaa");
    DBG_LOG("bbb");
    ERR_LOG("CCC");
    ERR_LOG("aaaa%d",333);
    return 0;
}