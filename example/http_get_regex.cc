#include<iostream>
#include<string>
#include<regex>


int main()
{
    std::string target = "GET /service/v2/device/data/history?devId=550e8400-e29b-41d4-a716-446655440000&start=2025-01-01&end=2025-12-31&fields=temp,humi,press&group=hour HTTP/1.1\r\n";
    std::regex rx("(GET|POST|HEAD|DELETE|PUT)\\s+([^?]*)(?:\\?(.*))?\\s+(HTTP/1\\.[01])(?:\n|\r\n)?");
    //std::regex rx("(GET|POST|HEAD|DELETE|PUT) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?");
    
    std::regex rx1("(GET|POST|HEAD|DELETE|PUT)\\s+(\\S+?)(?:\\?(\\S*))?\\s+(HTTP/1\\.[01])(?:\n|\r\n)?");

    std::smatch sm;
    std::regex_match(target,sm,rx);
    for(auto & e:sm)
    {
        std::cout<<e<<std::endl;
    }
    return 0;
}