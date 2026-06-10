#include <functional>

#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"

class Acceptor
{
private:
    Socket _socket;
    Channel _channel;
    EventLoop *_loop;
    using Acceptor_Callback_t = std::function<void(int)>;
    Acceptor_Callback_t _Acceptor_cb;
    int CreaterServer(uint16_t port)
    {
        int ret = _socket.CreateServerConnect(port, "0.0.0.0", true);
        assert(ret == true);
        return _socket.Get_fd();
    }
    void HanderRead()
    {
        int newfd = _socket.Accept();
        if (newfd < 0)
        {
            return;
        }
        if (_Acceptor_cb)
            _Acceptor_cb(newfd);
    }

public:
    Acceptor(EventLoop *loop, uint16_t port) : _socket(CreaterServer(port)), _loop(loop), _channel(_socket.Get_fd(), loop)
    {
        _channel.Set_Read_Callback(std::bind(&Acceptor::HanderRead, this));
    }
    ~Acceptor() {}
    void Set_Acceptor_Callback(const Acceptor_Callback_t &cb)
    {
        _Acceptor_cb = cb;
    }
    void listen() 
    {
        //回调要是晚于读事件监控设置,上面的第二个if会判断为假,无法执行,造成资源泄露
        _channel.Fd_Add_Read();
    }
};