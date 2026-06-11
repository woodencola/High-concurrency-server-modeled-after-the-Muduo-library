#pragma once
#include"LoopThreadPool.hpp"
#include"Acceptor.hpp"
#include"EventLoop.hpp"
#include"Connection.hpp"
#include<unordered_map>
#include<functional>

class TcpSever
{
    uint16_t _port;
    int _timeout;
    bool _Is_able_delay_del;//是否启用超时销毁
    int _Thread_cnt; //从属线程数量
    uint64_t _conn_id; // 连接唯一id
    std::shared_ptr<EventLoop> _base_loop; //主线程,用来接收所有的连接信息,这个是给accptor用的
    Acceptor _acceptor; //监听套接字组件
    LoopThreadPool _pool; //连接线程池
    std::unordered_map<uint64_t ,ConnPtr>  _conns; //连接管理
    using Conn_Connect_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Write_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Close_Callback = std::function<void(const ConnPtr &)>;
    using Conn_Event_Callback = std::function<void(const ConnPtr &)>;
    using Msg_Callback = std::function<void(const ConnPtr &, Buffer *)>; // 将数据放到输出缓冲区中(inbuffer) 业务处理
    using Func = std::function<void()>;
    Conn_Connect_Callback _Connect_Cb;
    Conn_Write_Callback _Write_Cb;
    Conn_Close_Callback _Close_Cb;
    Conn_Event_Callback _Event_Cb;
    Msg_Callback _Msg_Cb;

    void RemoveConnection(const ConnPtr& conn)
    {

    }

    void NewConnection(int fd)
    {
        
    }
    public:
    TcpSever();
    //服务器运行
    void Start(){}
    //设置从属线程数量
    void Set_Slave_Thread_Cnt(int cnt)
    {}
    //添加定时任务
    void RunAfter(const Func& f,int delay ){}
    //是否启动定时销毁
    void Enable_Is_Delay_del(int time){}
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