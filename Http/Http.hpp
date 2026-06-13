#include "../Sever_final/Sever.hpp"
#include <fstream>

class Uitl
{
public:
    // 字符串分割
    size_t split(const std::string &src, const std::string &sep, std::vector<std::string> *container)
    {
        // 空指针保护
        if (!container)
            return 0;
        // 空分隔符会导致死循环，直接返回0（不做任何分割）
        if (sep.empty())
            return 0;
        size_t offset = 0;
        while (offset < src.size())
        {
            size_t pos = src.find(sep, offset);
            if (pos == std::string::npos)
            {
                // 想当与后面没有了,直接全是
                container->push_back(src.substr(offset));
                return container->size();
            }
            // 如果pos的位置等于offset的话,就代表当前的是一个分割符,跳过它
            if (offset == pos)
            {
                offset = pos + sep.size();
                continue;
            }
            // 到这里代表当前特殊情况处理完直接开分解
            container->push_back(src.substr(offset, pos - offset));
            // 更新位置
            offset = pos + sep.size();
        }
        return container->size();
    }
    // 从文件中的读取内容
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
        // 空串直接返回
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
    // 向文件中写入数据
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
    // URL编码
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀% C++ -> C%2B2B
    //  不编码的特殊字符：RFC3986文档规定。- _ ~ 字母，数字属于绝对不编码字符
    // RFC3986文档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+，解码则是+转空格
    static std::string UrlEnCode(const std::string &src, bool is_space_to_plus)
    {
        std::string ret;
        for (auto &e : src)
        {
            if (e == '-' || e == '_' || e == '~' || e == '.' || isalnum(e))
            {
                ret += e;
            }
            else if (is_space_to_plus && e == ' ')
            {
                ret += '+';
            }
            else
            {
                char tmp[4] = {0};
                snprintf(tmp, 4, "%%%02X", e);
                ret += tmp;
            }
        }
        return ret;
    }
    // URL解码
    static bool UrlDecode();
    // 响应码的描述信息获取
    static std::string StatuDesc();
    // 根据文件后缀名获取mime
    static std::string ExMine();
    // 判断一个文件是否是一个目录
    static bool IsDirectory();
    // 判断一个文件是否是普通文件
    static bool IsRegular();
    // http请求的资源路径有效性判断
    static bool VaildPath();
};