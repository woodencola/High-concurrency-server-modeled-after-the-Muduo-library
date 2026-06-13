 #include "../Sever_final/Sever.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include<cctype>
//URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
//编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀% C++ -> C%2B2B
// 不编码的特殊字符：RFC3986文档规定。- _ ~ 字母，数字属于绝对不编码字符
//RFC3986文档规定，编码格式 %HH
//W3C标准中规定，查询字符串中的空格，需要编码为+，解码则是+转空格
 static std::string UrlEnCode(const std::string& src,bool is_space_to_plus)
 {
    std::string ret;
    for(auto& e:src)
    {
        if(e=='-'||e=='_'||e=='~'||e=='.'||isalnum(e))
        {
            ret+=e;
        }
        else if( is_space_to_plus&&e==' ')
        {
            ret+='+';
        }
        else
        {
            char tmp[4] = {0};
            snprintf(tmp,4,"%%%02X",e);
            ret+=tmp;
        }
    }
    return ret;
 }

 int main()
 {
    std::string Url = "/adadadadad?pwd=adaddaad&user=dadadadad        ";
    std::cout<<UrlEnCode(Url,true)<<std::endl;
    return 0;
 }