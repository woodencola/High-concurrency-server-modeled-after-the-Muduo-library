#include"../Sever_final/Sever.hpp"


class Uitl
{
    public:
    //字符串分割
    size_t spilt();
    //从文件中的读取内容
    static bool ReadFile();
    //向文件中写入数据
    static bool WriteFile();
    //URL编码
    static bool UrlEnCode();
    //URL解码
    static bool UrlDecode();
    //响应码的描述信息获取
    static std::string StatuDesc();
    //根据文件后缀名获取mime
    static std::string ExMine();
    //判断一个文件是否是一个目录
    static bool IsDirectory();
    //判断一个文件是否是普通文件
    static bool IsRegular();
    //http请求的资源路径有效性判断
    static bool VaildPath();
};