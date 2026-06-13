#include "../Sever_final/Sever.hpp"

#include <fstream>
#include <iostream>
#include <string>
static bool ReadFile(const std::string &filename, std::string *buffer)
{
    std::ifstream ifs;
    ifs.open(filename, std::ios::binary);
    if (!ifs.is_open())
    {
        ERR_LOG("OPEN FILE %s FAILED", filename.c_str());
        return false;
    }
    size_t typesize = 0;
    
    ifs.seekg(0, ifs.end);
    typesize = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    if (typesize == 0)
    {
        buffer->clear();
        return true;
    }
    buffer->resize(typesize);
    ifs.read(&(*buffer)[0], typesize);
    if (ifs.good() == false)
    {
        ERR_LOG("READ FILE %s FAILED", filename.c_str());
        ifs.close();
        return false;
    }
    ifs.close();
    return true;
}
static bool WriteFile(const std::string &filename, const std::string &buffer)
{
    std::ofstream ofs;
    ofs.open(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
    {
        ERR_LOG("OPEN FILE %s FAILED", filename.c_str());
        return false;
    }
    ofs.write(buffer.c_str(), buffer.size());
    if (ofs.good() == false)
    {
        ERR_LOG("WRITE FILE %s FAILED", filename.c_str());
        ofs.close();
        return false;
    }
    ofs.close();
    return true;
}

int main()

{
    std::string ret;

    if (ReadFile("/home/ubuntu/galgamesource/www.ymgal.games/index.html", &ret) == false)
    {
        return -1;
    }
    if(WriteFile("../Http/index.html",ret)==false)
    {
        return -1;
    }
    return 0;
}