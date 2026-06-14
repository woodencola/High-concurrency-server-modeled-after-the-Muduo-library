#pragma once
#include "../Sever_final/Sever.hpp"
#include <fstream>
#include <sys/stat.h>
#include <regex>
class Uitl
{
public:
    // 动态重写 HTML 中的链接
    // html: 原始 HTML 内容
    // target_domain: 需要替换的原始域名（例如 "https://www.ymgal.games"）
    // local_prefix: 本地前缀（通常为 "/" 或 "http://你的IP:1315"）
    // exclude_extensions: 不替换的扩展名列表（如图片、字体等）
    // static std::string RewriteHtmlLinks(const std::string &html,
    //                                     const std::string &target_domain,
    //                                     const std::string &local_prefix = "/",
    //                                     const std::vector<std::string> &exclude_extensions = {".jpg", ".jpeg", ".png", ".gif", ".webp", ".svg", ".ico", ".mp4", ".webm", ".pdf"})
    // {
    //     // 匹配 href="http://..." 或 src="http://..."  注意：仅匹配双引号内的绝对 URL
    //     static std::regex url_regex(R"((href|src)=\"(https?://[^\"]+)\")");
    //     std::string result;
    //     auto it = std::sregex_iterator(html.begin(), html.end(), url_regex);
    //     auto end = std::sregex_iterator();
    //     size_t last_pos = 0;
    //     for (; it != end; ++it)
    //     {
    //         const std::smatch &match = *it;
    //         result.append(html, last_pos, match.position() - last_pos);
    //         std::string attr = match[1].str(); // "href" 或 "src"
    //         std::string url = match[2].str();  // 完整 URL

    //         // 是否属于需要替换的域名
    //         bool need_replace = (url.find(target_domain) == 0);
    //         if (need_replace)
    //         {
    //             // 检查是否排除某些扩展名（即仍然保持原链接）
    //             bool excluded = false;
    //             for (const auto &ext : exclude_extensions)
    //             {
    //                 if (url.length() > ext.length() && url.substr(url.length() - ext.length()) == ext)
    //                 {
    //                     excluded = true;
    //                     break;
    //                 }
    //             }
    //             if (!excluded)
    //             {
    //                 // 提取路径部分：去掉域名，保留 /path?query...
    //                 std::string path = url.substr(target_domain.length());
    //                 if (path.empty())
    //                     path = "/";
    //                 std::string new_url = local_prefix + path;
    //                 result += attr + "=\"" + new_url + "\"";
    //             }
    //             else
    //             {
    //                 // 排除的资源（如图片）保持原链接
    //                 result += attr + "=\"" + url + "\"";
    //             }
    //         }
    //         else
    //         {
    //             // 非目标域名的链接原样保留
    //             result += attr + "=\"" + url + "\"";
    //         }
    //         last_pos = match.position() + match.length();
    //     }
    //     result.append(html, last_pos, html.size() - last_pos);
    //     return result;
    // }
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
    // http请求的资源路径有效性判断
    static bool VaildPath(const std::string &path)
    {
        std::vector<std::string> v;
        split(path, "/", &v);
        int level = 0;
        for (int i = 0; i < v.size(); i++)
        {
            if (v[i] == "..")
            {
                level--;
                if (level < 0)
                {
                    return false;
                }
            }
            else
                level++;
        }
        return true;
    }
};
class HttpRequest
{
public:
    // 请求方法
    std::string _method;
    // 请求路径
    std::string _path;
    // 请求版本
    std::string _version;
    // 请求正文
    std::string _body;
    // 正则获取的结果
    std::smatch _matches;
    // 请求行
    std::unordered_map<std::string, std::string> _headers;
    // 查询字符串
    std::unordered_map<std::string, std::string> _params;

public:
    void ReSet()
    {
        _method.clear();
        _path.clear();
        _version.clear();
        _body.clear();
        std::smatch tmp;
        _matches.swap(tmp);
        _headers.clear();
        _params.clear();
    }
    // 设置请求行
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    // 判断请求行是否存在
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        return false;
    }
    std::string GetHeader(const std::string &key)
    {
        // 获取请求行
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return it->second;
        }
        return std::string();
    }
    void SetParams(const std::string &key, const std::string &val)
    {
        _params.insert(std::make_pair(key, val));
    }
    bool HasParams(std::string &key)
    {
        auto it = _params.find(key);
        if (it != _params.end())
        {
            return true;
        }
        return false;
    }
    std::string GetParams(const std::string &key)
    {
        // 获取查询字符串
        auto it = _params.find(key);
        if (it != _params.end())
        {
            return it->second;
        }
        return std::string();
    }
    size_t BodySize()
    {
        // 正文长度
        std::string length = "Content-Length";
        if (!HasHeader(length))
        {
            return 0;
        }
        return std::stol(GetHeader(length));
    }
    bool Is_Cose()
    {
        // 是否是短链接
        std::string connection_type = "Connection";
        if (HasHeader(connection_type) && GetHeader(connection_type) == "keep-alive")
        {
            return false;
        }
        return true;
    }
};
class HttpResponse
{
public:
    // 状态码描述
    int _status;
    // 正文
    std::string _body;
    // 是否重定向
    bool Is_Redirect;
    // 重定向路径
    std::string Redirect;
    std::unordered_map<std::string, std::string> _headers;

public:
    HttpResponse() : _status(200), Is_Redirect(false) {}
    HttpResponse(int status) : _status(status), Is_Redirect(false) {}
    // 设置请求行
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    // 判断请求行是否存在
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        return false;
    }
    std::string GetHeader(const std::string &key)
    {
        // 获取请求行
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return it->second;
        }
        return std::string();
    }
    // 重置
    void Reset()
    {
        //      int _status;
        // // 正文
        // std::string _body;
        // // 是否重定向
        // bool Is_Redirect;
        // // 重定向路径
        // std::string Redirect;
        // std::unordered_map<std::string, std::string> _headers;
        _status = 200;
        _body.clear();
        Is_Redirect = false;
        Redirect.clear();
        _headers.clear();
    }
    void Setbody(const std::string &text, const std::string &type)
    {
        // 设置相应正文
        SetHeader("Content-Type", type);
        _body = text;
    }
    void SetRedirect(const std::string &url, int stau = 302)
    {
        // 设置重定向
        _status = stau;

        Is_Redirect = true;
        Redirect = url;
    }
    bool Is_Cose()
    {
        // 是否是短链接
        std::string connection_type = "Connection";
        if (HasHeader(connection_type) && GetHeader(connection_type) == "keep-alive")
        {
            return false;
        }
        return true;
    }
};
typedef enum
{
    RECV_HTTP_ERR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatu;
#define MAX_LINE 8192
#define MAX_BODY_SIZE (2 * 1024 * 1024)
class HttpContent
{
private:
    int _resq_statu;           // 响应状态码
    HttpRecvStatu _recv_statu; // 当前处理的状态
    HttpRequest _request;

private:
    // 解析请求行
    bool ParseHttpLine(const std::string &buffer)
    {

        // GET /service/v2/device/data/history?devId=550e8400-e29b-41d4-a716-446655440000&start=2025-01-01&end=2025-12-31&fields=temp,humi,press&group=hour HTTP/1.1

        // GET
        /// service/v2/device/data/history
        // devId=550e8400-e29b-41d4-a716-446655440000&start=2025-01-01&end=2025-12-31&fields=temp,humi,press&group=hour
        // HTTP/1.1
        static std::regex rx1("(GET|POST|HEAD|DELETE|PUT)\\s+(\\S+?)(?:\\?(\\S*))?\\s+(HTTP/1\\.[01])(?:\n|\r\n)?");
        std::smatch sm;
        bool ret = std::regex_match(buffer, sm, rx1);
        if (ret == false)
        {
            _resq_statu = 400; // bed request;
            _recv_statu = RECV_HTTP_ERR;
            return false;
        }
        _request._version = sm[4];
        _request._method = sm[1];
        _request._path = Uitl::UrlDecode(sm[2], false);
        std::string sreach = sm[3];
        std::vector<std::string> v;
        Uitl::split(sreach, "&", &v);
        for (auto &e : v)
        {
            int pos = e.find('=');
            if (pos == std::string::npos)
            {
                _resq_statu = 400; // bed request;
                _recv_statu = RECV_HTTP_ERR;
                return false;
            }
            std::string key = Uitl::UrlDecode(e.substr(0, pos), true);
            std::string value = Uitl::UrlDecode(e.substr(pos + 1), true);
            _request.SetParams(key, value);
        }

        return true;
    }
    // 接收请求行
    bool RecvHttpLine(Buffer *buffer)
    {
        if (_recv_statu != RECV_HTTP_LINE)
            return false;
        std::string line = buffer->GetLineAndAdd();
        if (line.size() == 0)
        {
            // 走到这里代表没用接收到一个完成的请求行
            if (buffer->CurrentEnableReadSpaceSize() > MAX_LINE)
            {
                // 走到这里代表当前请求行太长不处理
                _resq_statu = 414;
                _recv_statu = RECV_HTTP_ERR;
                return false;
            }
            // 不够继续收
            return true;
        }
        if (line.size() > MAX_LINE)
        {
            // 走到这里代表当前请求行太长不处理
            _resq_statu = 414;
            _recv_statu = RECV_HTTP_ERR;

            return false;
        }
        bool ret = ParseHttpLine(line);
        _recv_statu = RECV_HTTP_HEAD;
        return ret;
    }
    // 接收头部
    bool RecvHttpHead(Buffer *buffer)
    {
        if (_recv_statu != RECV_HTTP_HEAD)
            return false;
        while (1)
        {
            std::string line = buffer->GetLineAndAdd();
            if (line.size() == 0)
            {
                // 走到这里代表没用接收到一个完成的请求行
                if (buffer->CurrentEnableReadSpaceSize() > MAX_LINE)
                {
                    // 走到这里代表当前请求行太长不处理
                    _resq_statu = 414;
                    _recv_statu = RECV_HTTP_ERR;
                    return false;
                }
                // 不够继续收
                return true;
            }
            if (line.size() > MAX_LINE)
            {
                // 走到这里代表当前请求行太长不处理
                _resq_statu = 414;
                _recv_statu = RECV_HTTP_ERR;

                return false;
            }
            if (line == "\n" || line == "\r\n")
            {
                break;
            }
            bool ret = ParseHttpHead(line);
            if (ret == false)
            {
                return false;
            }
        }
        _recv_statu = RECV_HTTP_BODY;
        return true;
    }
    // 解析头部
    bool ParseHttpHead(std::string &line)
    {

        if (line.back() == '\n')
            line.pop_back(); // 末尾是换行则去掉换行字符
        if (line.back() == '\r')
            line.pop_back(); // 末尾是回车则去掉回车字符
        int pos = line.find(": ");
        if (pos == std::string::npos)
        {
            _resq_statu = 400; // bed request;
            _recv_statu = RECV_HTTP_ERR;
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2);
        _request.SetHeader(key, value);
        return true;
    }
    // 接收正文
    bool RecvHttpBody(Buffer *buffer)
    {

        if (_recv_statu != RECV_HTTP_BODY)
            return false;
        // 1.获取正文长度
        size_t Contentlength = _request.BodySize();
        // // 防御：已有 body 超过声明长度
        // if (_request._body.size() > Contentlength)
        // {
        //     _recv_statu = RECV_HTTP_ERR;
        //     _resq_statu = 400;
        //     return false;
        // }
        // ：限制最大 body 大小
        // if (Contentlength > MAX_BODY_SIZE)
        // {
        //     _recv_statu = RECV_HTTP_ERR;
        //     _resq_statu = 413;
        //     return false;
        // }
        if (Contentlength == 0)
        {
            // 接收完毕
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }
        // 2.判断已经接收了多少正文
        size_t real_size = Contentlength - _request._body.size();

        // 判断是否已经接收了一个完整的报文
        if (buffer->CurrentEnableReadSpaceSize() >= real_size)
        {
            _request._body.append(buffer->GetCurrentReadPosition(), real_size);
            buffer->MoveReadPosition(real_size);
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }
        // 去出缓冲区的当中的数据,按需获取
        else
        {
            _request._body.append(buffer->GetCurrentReadPosition(), buffer->CurrentEnableReadSpaceSize());
            buffer->MoveReadPosition(buffer->CurrentEnableReadSpaceSize());
        }
        // 不够,就取出全部
        return true;
    }

public:
    HttpContent() : _recv_statu(RECV_HTTP_LINE), _resq_statu(200) {}
    // 获取状态码
    int resqstatu()
    {
        return _resq_statu;
    };
    // 获取当前的处理状态
    HttpRecvStatu RecvStatu()
    {
        return _recv_statu;
    }
    // 获取当前的请求
    HttpRequest &Request()
    {
        return _request;
    }
    // 解析
    void RecvHttpRequest(Buffer *buffer)
    {
        switch (_recv_statu)
        {
        case RECV_HTTP_LINE:
            RecvHttpLine(buffer);
        case RECV_HTTP_HEAD:
            RecvHttpHead(buffer);
        case RECV_HTTP_BODY:
            RecvHttpBody(buffer);
        }
        return;
    }
};
class Http_Server
{
    private:
    using Handler = std::function<void(const HttpRequest& ,HttpResponse*)>;
    std::unordered_map<std::string,Handler> _get_route;
    std::unordered_map<std::string,Handler> _post_route;
    std::unordered_map<std::string,Handler> _put_route;
    std::unordered_map<std::string,Handler> _delete_route;
    std::string _basedir;//web根目录
    TcpSever _server;
    void WriteResponse(); //将response种的数据按照一定的http响应格式发送
    void FileHandler(); //静态分发
    void Route();
    void OnConnected();//设置上下文
    void OnMessage();//缓冲区数据解析+处理
    public:
    Http_Server();
    void SetBasedir(const std::string& path);
    void Get(const std::string& pattern,Handler & handler);
    void Put(const std::string& pattern,Handler & handler);
    void Post(const std::string& pattern,Handler & handler);
    void Delete(const std::string& pattern,Handler & handler);
    void SetThreadCount(int count);
    void EnableInactiveRelease(int timeout);
    void Listen();
};