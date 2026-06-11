#pragma once
#include "LoopThreadPool.hpp"
#include "Acceptor.hpp"
#include "EventLoop.hpp"
#include "Connection.hpp"
#include <unordered_map>
#include <functional>

class TcpSever
{
    uint16_t _port;
    int _timeout;
    bool _Is_able_delay_del;                      // 是否启用超时销毁
    int _Thread_cnt;                              // 从属线程数量
    uint64_t _conn_id;                            // 连接唯一id
    std::shared_ptr<EventLoop> _base_loop;        // 主线程,用来接收所有的连接信息,这个是给accptor用的
    Acceptor _acceptor;                           // 监听套接字组件
    LoopThreadPool _pool;                         // 连接线程池
    std::unordered_map<uint64_t, ConnPtr> _conns; // 连接管理
    using Conn_Connect_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>; // 将数据放到输出缓冲区中(inbuffer) 业务处理
    using Function = std::function<void()>;
    Conn_Connect_Callback _Connect_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    Msg_Callback _Msg_Cb;

    void RemoveConnectionInLoop(const ConnPtr &conn)
    {
        uint64_t id = conn->Get_Id();
        auto it = _conns.find(id);
        if(it !=_conns.end())
        {
            _conns.erase(id);
        }
    }
    void RemoveConnection(const ConnPtr &conn)
    {
        _base_loop->RunInLoop(std::bind(&TcpSever::RemoveConnectionInLoop,this,conn));
    }

    void NewConnection(int fd)
    {
        _conn_id++;
        ConnPtr conn = std::make_shared<Connection>(_conn_id,fd,_pool.Get_Next_EventLoop());
        conn->Set_Msg_Callback(_Msg_Cb);
        conn->Set_Conn_Connect_Callback(_Connect_Cb);
        conn->Set_Conn_Close_Callback(_Close_Cb);
        conn->Set_Conn_Event_Callback(_Event_Cb);
        conn->Set_Server_Callback(std::bind(&TcpSever::RemoveConnection,this,std::placeholders::_1));
        if(_Is_able_delay_del)
        conn->EnableTimeoutDel(10);
        conn->Established();
        _conns.insert(std::make_pair(_conn_id,conn));
    }
    void RunAfterInLoop(const Function &f, int delay)
    {
        _conn_id++;
        _base_loop->TimerAdd(_conn_id,delay,f);
    }

public:
    TcpSever(int port) : _port(port), _timeout(0), _Is_able_delay_del(false), _Thread_cnt(0),
                         _conn_id(0), _base_loop(std::make_shared<EventLoop>()), _acceptor(_base_loop.get(), _port), _pool(_base_loop.get())
    {
        // 设置监听套接字回调
        _acceptor.Set_Acceptor_Callback(std::bind(&TcpSever::NewConnection, this, std::placeholders::_1));
         _acceptor.listen();
    }
    // 服务器运行
    void Start()
    {
        // 创建从属线程
        _pool.Create();
        // 设置监听
       
        _base_loop->Start();
    }
    // 设置从属线程数量
    void Set_Slave_Thread_Cnt(int cnt)
    {
        _Thread_cnt = cnt;
    }
    // 添加定时任务
    void RunAfter(const Function &f, int delay)
    {
        _base_loop->RunInLoop(std::bind(&TcpSever::RunAfterInLoop, this, f, delay));
    }
    // 是否启动定时销毁
    void Enable_Is_Delay_del(int time)
    {
        _Is_able_delay_del = true;
        _timeout = time;
    }
    void Set_Conn_Connect_Callback(const Conn_Connect_Callback &cb)
    {
        _Connect_Cb = cb;
    }
    //(以废弃)
    void Set_Conn_Write_Callback(const Conn_Write_Callback &cb)
    {
        _Write_Cb = cb;
    }
    void Set_Conn_Close_Callback(const Conn_Close_Callback &cb)
    {
        _Close_Cb = cb;
    }
    void Set_Conn_Event_Callback(const Conn_Event_Callback &cb)
    {
        _Event_Cb = cb;
    }
    void Set_Msg_Callback(const Msg_Callback &cb)
    {
        _Msg_Cb = cb;
    }
};