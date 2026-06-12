#include<iostream>
#include<string>
#include<vector>


size_t split(const std::string & src,const std::string& sep,std::vector<std::string>* container)
{
     // 空指针保护
    if (!container) return 0;
    // 空分隔符会导致死循环，直接返回0（不做任何分割）
    if (sep.empty()) return 0;
    size_t  offset = 0;
    while(offset<src.size())
    {
        size_t pos = src.find(sep,offset);
        if(pos==std::string::npos)
        {
            //想当与后面没有了,直接全是
            container->push_back(src.substr(offset));
            return container->size();
        }
        //如果pos的位置等于offset的话,就代表当前的是一个分割符,跳过它
        if(offset==pos)
        {
            offset=pos+sep.size();
            continue;
        }
        //到这里代表当前特殊情况处理完直接开分解
        container->push_back(src.substr(offset,pos-offset));
        //更新位置
        offset = pos+sep.size();
    }
    return container->size();
}



int main()
{
    std::string s = "abc,def,,,,,,,,,,,,hij,";
    std::vector<std::string> v;
    split(s,",",&v);
    for(auto& e:v)
    {
        std::cout<<e<<std::endl;
    }
    return 0;
}