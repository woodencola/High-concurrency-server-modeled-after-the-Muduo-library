#include<iostream>
#include<string>
#include<regex>


int main()
{
    std::string a("/a/123123124");

    std::regex b("/a/(\\d+)");
    std::smatch sm;
    std::regex_match(a,sm,b);
    for(auto & e:sm)
    {
        std::cout<<e<<std::endl;
    }
    return 0;
}