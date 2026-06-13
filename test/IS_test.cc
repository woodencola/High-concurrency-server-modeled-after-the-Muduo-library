 #include "../Sever_final/Sever.hpp"
 #include"../Http/Http.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <cctype>
#include<sys/stat.h>
// URL编码，避免URL中资
static bool IsDirectory(const std::string &filename)
{
    struct stat st;
    int ret = stat(filename.c_str(), &st);
    if (ret < 0)
    {
        ERR_LOG("FILE NOT EXIST");
        return false;
    }
    return S_ISDIR(st.st_mode);
}
// 判断一个文件是否是普通文件
static bool IsRegular(const std::string &filename)
{

    struct stat st;
    int ret = stat(filename.c_str(), &st);
    if (ret < 0)
    {
        ERR_LOG("FILE NOT EXIST");
        return false;
    }
    return S_ISREG(st.st_mode);
}
int main()
{
    std::string s="/../../../a/v/c/c";
    
    std::cout<<Uitl::VaildPath(s)<<std::endl;
    std::cout<<IsDirectory("testtest")<<std::endl;
    std::cout<<IsDirectory("log_test.cc")<<std::endl;
    std::cout<<IsRegular("testtest")<<std::endl;
    std::cout<<IsRegular("log_test.cc")<<std::endl;

}