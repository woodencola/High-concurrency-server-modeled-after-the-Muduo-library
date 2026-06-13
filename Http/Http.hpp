#include "../Sever_final/Sever.hpp"
#include <fstream>

class Uitl
{
public:
    // 字符串分割
    static size_t split(const std::string &src, const std::string &sep, std::vector<std::string> *container)
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
                //  比如汉字 中，它的 UTF-8 编码是三个字节：E4 B8 AD（十进制：228, 184, 173）。
                //  这三个字节的值都大于 127，超出了 ASCII 范围。
                // 当把 228 赋给 char 变量时，会发生溢出，实际存储的值是 228 - 256 = -28。

                // 如果用 %02X 打印 -28（先转为 int，符号扩展到 32 位），会得到 FFFFFFE4 而不是期望的 E4。

                // 最终编码出来的字符串会变成 %FFFFFFE4 而不是 %E4，浏览器无法解码。
                unsigned char c = static_cast<unsigned char>(e);
                snprintf(tmp, 4, "%%%02X", c);
                ret += tmp;
            }
        }
        return ret;
    }
    // URL解码
    static int CONV(char a)
    {
        if (a >= '0' && a <= '9')
        {
            return a - '0';
        }
        else if (a >= 'a' && a <= 'z')
        {
            return a - 'a' + 10;
        }
        else if (a >= 'A' && a <= 'Z')
        {
            return a - 'A' + 10;
        }
        return -1;
    }
    static std::string UrlDecode(const std::string &src, bool Is_space_To_plus)
    {
        std::string ret;
        for (int i = 0; i < src.size(); i++)
        {
            if (src[i] == '+' && Is_space_To_plus)
            {
                ret += ' ';
            }
            else if (src[i] == '%')
            {
                if (i + 2 >= src.size())
                    break; // 不足则终止
                // 第一个数字左移4位加上第二个数字
                int first = CONV(src[i + 1]);
                int second = CONV(src[i + 2]);
                int tmp = (first << 4) + second;
                ret += tmp;
                i += 2;
            }
            else
            {
                ret += src[i];
            }
        }
        return ret;
    }
    // 响应码的描述信息获取
    static const std::string &StatuDesc(int code)
    {
        static std::unordered_map<int, std::string> map{
            {100, "Continue"},
            {101, "Switching Protocol"},
            {102, "Processing"},
            {103, "Early Hints"},
            {200, "OK"},
            {201, "Created"},
            {202, "Accepted"},
            {203, "Non-Authoritative Information"},
            {204, "No Content"},
            {205, "Reset Content"},
            {206, "Partial Content"},
            {207, "Multi-Status"},
            {208, "Already Reported"},
            {226, "IM Used"},
            {300, "Multiple Choice"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {303, "See Other"},
            {304, "Not Modified"},
            {305, "Use Proxy"},
            {306, "unused"},
            {307, "Temporary Redirect"},
            {308, "Permanent Redirect"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {402, "Payment Required"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {406, "Not Acceptable"},
            {407, "Proxy Authentication Required"},
            {408, "Request Timeout"},
            {409, "Conflict"},
            {410, "Gone"},
            {411, "Length Required"},
            {412, "Precondition Failed"},
            {413, "Payload Too Large"},
            {414, "URI Too Long"},
            {415, "Unsupported Media Type"},
            {416, "Range Not Satisfiable"},
            {417, "Expectation Failed"},
            {418, "I'm a teapot"},
            {421, "Misdirected Request"},
            {422, "Unprocessable Entity"},
            {423, "Locked"},
            {424, "Failed Dependency"},
            {425, "Too Early"},
            {426, "Upgrade Required"},
            {428, "Precondition Required"},
            {429, "Too Many Requests"},
            {431, "Request Header Fields Too Large"},
            {451, "Unavailable For Legal Reasons"},
            {501, "Not Implemented"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"},
            {504, "Gateway Timeout"},
            {505, "HTTP Version Not Supported"},
            {506, "Variant Also Negotiates"},
            {507, "Insufficient Storage"},
            {508, "Loop Detected"},
            {510, "Not Extended"},
            {511, "Network Authentication Required"},

        };
        auto it = map.find(code);
        if (it != map.end())
        {
            return it->second;
        }
        static const std::string unknown = "unknown";
        return unknown;
    }
    // 根据文件后缀名获取mime
    static const std::string &ExMime(const std::string &filename)
    {
        static std::unordered_map<std::string, std::string> mime{
            {".aac", "audio/aac"},
            {".abw", "application/x-abiword"},
            {".arc", "application/x-freearc"},
            {".avi", "video/x-msvideo"},
            {".azw", "application/vnd.amazon.ebook"},
            {".bin", "application/octet-stream"},
            {".bmp", "image/bmp"},
            {".bz", "application/x-bzip"},
            {".bz2", "application/x-bzip2"},
            {".csh", "application/x-csh"},
            {".css", "text/css"},
            {".csv", "text/csv"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".eot", "application/vnd.ms-fontobject"},
            {".epub", "application/epub+zip"},
            {".gif", "image/gif"},
            {".htm", "text/html"},
            {".html", "text/html"},
            {".ico", "image/vnd.microsoft.icon"},
            {".ics", "text/calendar"},
            {".jar", "application/java-archive"},
            {".jpeg", "image/jpeg"},
            {".jpg", "image/jpeg"},
            {".js", "text/javascript"},
            {".json", "application/json"},
            {".jsonld", "application/ld+json"},
            {".mid", "audio/midi"},
            {".midi", "audio/x-midi"},
            {".mjs", "text/javascript"},
            {".mp3", "audio/mpeg"},
            {".mpeg", "video/mpeg"},
            {".mpkg", "application/vnd.apple.installer+xml"},
            {".odp", "application/vnd.oasis.opendocument.presentation"},
            {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
            {".odt", "application/vnd.oasis.opendocument.text"},
            {".oga", "audio/ogg"},
            {".ogv", "video/ogg"},
            {".ogx", "application/ogg"},
            {".otf", "font/otf"},
            {".png", "image/png"},
            {".pdf", "application/pdf"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
            {".rar", "application/x-rar-compressed"},
            {".rtf", "application/rtf"},
            {".sh", "application/x-sh"},
            {".svg", "image/svg+xml"},
            {".swf", "application/x-shockwave-flash"},
            {".tar", "application/x-tar"},
            {".tif", "image/tiff"},
            {".tiff", "image/tiff"},
            {".ttf", "font/ttf"},
            {".txt", "text/plain"},
            {".vsd", "application/vnd.visio"},
            {".wav", "audio/wav"},
            {".weba", "audio/webm"},
            {".webm", "video/webm"},
            {".webp", "image/webp"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".xhtml", "application/xhtml+xml"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {".xml", "application/xml"},
            {".xul", "application/vnd.mozilla.xul+xml"},
            {".zip", "application/zip"},
            {".3gp", "video/3gpp"},
            {".3g2", "video/3gpp2"},
            {".7z", "application/x-7z-compressed"},
        };
        static std::string ocstream = "application/octet-stream";
        int pos = filename.rfind('.');
        if (pos == std::string::npos)
        {

            return ocstream;
        }
        std::string sub = filename.substr(pos);
        for (char &c : sub)
            c = tolower(c);
        auto it = mime.find(sub);
        if (it == mime.end())
        {
            // 没找到返回一个流对象
            return ocstream;
        }
        return it->second;
    }
    // 判断一个文件是否是一个目录
    static bool IsDirectory();
    // 判断一个文件是否是普通文件
    static bool IsRegular();
    // http请求的资源路径有效性判断
    static bool VaildPath();
};