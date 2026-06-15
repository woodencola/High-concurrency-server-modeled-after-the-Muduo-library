#include <iostream>
#include <string>
#include <unistd.h>
#include <assert.h>
#include "../source/Socket.hpp"

int main()
{
    Socket client;
    client.CreateClientConnect(1314, "127.0.0.1");
    std::string ret = "GET /hello HTTP/1.1\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
    while (1)
    {
        assert(client.Send(ret.c_str(), ret.size())!=-1);
        char buf[1024] = {0};
        assert(client.Recv(buf, 1023));
        DBG_LOG("[%s]", buf);
        sleep(15);
    }
    return 0;
}