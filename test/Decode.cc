#include <fstream>
#include <iostream>
#include <string>
#include <cctype>
int CONV(char a)
{
    if(a>='0'&&a<='9')
    {
        return a-'0';
    }
    else if(a>='a'&&a<='z')
    {
        return a-'a'+10;
    }
    else if(a>='A'&&a<='Z')
    {
        return a-'A'+10;
    }
    return -1;
    
}
static std::string UrlDecode(const std::string & src,bool Is_space_To_plus)
{
    std::string ret;
    for(int i= 0;i<src.size();i++)
    {
        if(src[i]=='+'&&Is_space_To_plus)
        {
            ret+=' ';
        }
        else if(src[i]=='%')
        {
            if (i + 2 >= src.size()) break; // 不足则终止
            //第一个数字左移4位加上第二个数字
            int first = CONV(src[i+1]);
            int second =CONV(src[i+2]);
            int tmp  = (first<<4)+second;
            ret+=tmp;
            i+=2;
        }
        else
        {
            ret+=src[i];
        }
    }
    return ret;
}

int main()
{
    std::string target = "%2Fadadadadad%3Fpwd%3Dadaddaad%26user%3Ddadadadad%20%20%20%20%20%20%20%20";
    std::cout<<UrlDecode(target,false)<<std::endl;
    return 0;
}