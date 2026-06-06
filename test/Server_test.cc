#include<iostream>
#include"../source/Socket.hpp"

int main()
{
    Socket Server;
    Server.CreateServerConnect(1314);
    while(1)
    {
        int fd = Server.Accept();
        Socket ser(fd);
        char buffer[1024] = {0};
        int ret = ser.Recv(buffer,sizeof (buffer)-1);
        if(ret<0)
        {
            ser.Close();
            continue;
        }
        ser.Send(buffer,sizeof(buffer)-1);
        ser.Close();
    }
    Server.Close();
    return 0;
}