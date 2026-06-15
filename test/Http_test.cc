
#include "../Http/Http.hpp"

std::string trans(const HttpRequest &req)
{
    std::stringstream ss;
    ss << req._method<< " " << req._path << " " << req._version<< "\r\n";
    for (auto &it :req._params)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    
    for (auto &it : req._headers)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}
void Hello(const HttpRequest &req, HttpResponse *res)
{
    res->Setbody(trans(req),"text/plain");

}
void put(const HttpRequest &req, HttpResponse *res)
{
    res->Setbody(trans(req),"text/plain");
    
}
void delete1(const HttpRequest &req, HttpResponse *res)
{
    res->Setbody(trans(req),"text/plain");
    
}
void post(const HttpRequest &req, HttpResponse *res)
{
    res->Setbody(trans(req),"text/plain");
    
}
int main()
{
    Http_Server tsr(1314);
    tsr.SetBasedir("./wwwroot/");
    tsr.Get("/hello", Hello);
    tsr.Post("/login",post);
    tsr.Delete("/1234.txt",delete1);
    tsr.Put("/1234.txt",put);
    tsr.SetThreadCount(3);
    tsr.Listen();
    return 0;
}