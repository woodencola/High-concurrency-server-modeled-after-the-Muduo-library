#include"../Sever_final/Sever.hpp"
#include<fstream>
#include<iostream>
#include<string>
 static bool ReadFile(const std::string& filename,std::string* buffer)
 {
    std::ifstream  ifs;
    ifs.open(filename,std::ios::binary);
    if(!ifs.is_open())
    {
        ERR_LOG("OPEN FILE %s FAILED",filename.c_str());
        return false;
    }
    size_t typesize = 0;
    if (typesize == 0) {
        buffer->clear();
        return true;
    }
    ifs.seekg(0,ifs.end);
    typesize = ifs.tellg();
    ifs.seekg(0,ifs.beg);
    buffer->resize(typesize);
    ifs.read(&(*buffer)[0],typesize);
    if(ifs.good()==false)
    {
        ERR_LOG("READ FILE %s FAILED",filename.c_str());
        ifs.close();
        return false;
    }
    ifs.close();
    return true;
 }
int main()
{
    std::string ret;
    
    if(ReadFile("/home/ubuntu/galgamesource/www.ymgal.games/index.html",&ret)==false)
    {
        return -1;
    }
    std::cout<<ret<<std::endl;
    return 0;

}