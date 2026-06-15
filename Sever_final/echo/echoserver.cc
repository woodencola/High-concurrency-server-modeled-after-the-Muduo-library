// main.cpp
#include "../Sever.hpp"   
#include "../../Http/Http.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <functional>

class HttpServer {
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

    explicit HttpServer(uint16_t port, const std::string& root_dir)
        : _tcp_sever(port), _root_dir(root_dir) {
        _tcp_sever.Set_Msg_Callback(
            std::bind(&HttpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        _tcp_sever.Set_Conn_Connect_Callback([](const ConnPtr& conn) {
            DBG_LOG("New connection from fd=%d", conn->Get_Fd());
        });
        _tcp_sever.Set_Conn_Close_Callback([](const ConnPtr& conn) {
            DBG_LOG("Connection closed: fd=%d", conn->Get_Fd());
        });
    }

    void Start(int thread_cnt = 0) {
        _tcp_sever.Set_Slave_Thread_Cnt(thread_cnt);
        _tcp_sever.Start();
    }

private:
    TcpSever _tcp_sever;
    std::string _root_dir;

    struct ConnContext {
        HttpContent parser;
        bool keep_alive = true;
    };

    void OnMessage(const ConnPtr& conn, Buffer* buffer) {
        ConnContext* ctx = conn->Get_Context()->Get<ConnContext>();
        if (!ctx) {
            ctx = new ConnContext();
            conn->Set_Context(ctx);
        }

        HttpContent& parser = ctx->parser;
        parser.RecvHttpRequest(buffer);
        int status = parser.resqstatu();

        if (status != 200) {
            HttpResponse resp(status);
            BuildErrorResponse(resp);
            SendResponse(conn, resp);
            conn->Shutdown();
            delete ctx;
            conn->Set_Context(nullptr);
            return;
        }

        if (parser.RecvStatu() == RECV_HTTP_OVER) {
            HttpRequest& req = parser.Request();
            HttpResponse resp;
            HandleRequest(req, resp);

            bool client_keep = false;
            if (req._version == "HTTP/1.1") {
                client_keep = (req.GetHeader("Connection") != "close");
            } else {
                client_keep = (req.GetHeader("Connection") == "keep-alive");
            }
            ctx->keep_alive = client_keep;
            if (ctx->keep_alive) {
                resp.SetHeader("Connection", "keep-alive");
            } else {
                resp.SetHeader("Connection", "close");
            }

            SendResponse(conn, resp);
            parser = HttpContent();
            if (!ctx->keep_alive) {
                conn->Shutdown();
                delete ctx;
                conn->Set_Context(nullptr);
            }
        }
    }

  void HandleRequest(const HttpRequest& req, HttpResponse& resp) {
    // 只支持 GET 和 HEAD
    if (req._method != "GET" && req._method != "HEAD") {
        resp._status = 405;
        BuildErrorResponse(resp);
        return;
    }

    std::string path = req._path;

    // 防止路径遍历攻击
    if (!Uitl::VaildPath(path)) {
        resp._status = 403;
        BuildErrorResponse(resp);
        return;
    }

    // 处理根路径
    if (path.empty() || path == "/") {
        path = "/index.html";
    }

    // 构建完整文件系统路径
    std::string full_path = _root_dir + path;

    // ---------- 1. 先尝试直接作为文件 ----------
    bool is_file = Uitl::IsRegular(full_path);

    // ---------- 2. 如果不是文件且路径无后缀，尝试加 .html ----------
    if (!is_file && path.find('.') == std::string::npos && path.back() != '/') {
        std::string try_path = full_path + ".html";
        if (Uitl::IsRegular(try_path)) {
            full_path = try_path;
            is_file = true;
        }
    }

    // ---------- 3. 如果是目录且不以斜杠结尾，重定向 ----------
    if (!is_file && Uitl::IsDirectory(full_path) && path.back() != '/') {
        resp.SetRedirect(path + "/");
        return;
    }

    // ---------- 4. 如果以斜杠结尾，补 index.html ----------
    if (path.back() == '/') {
        full_path += "index.html";
        is_file = Uitl::IsRegular(full_path);
    }

    // ---------- 5. 最终判断 ----------
    if (!is_file) {
        resp._status = 404;
        BuildErrorResponse(resp);
        return;
    }

    // 读取文件内容
    std::string content;
    if (!Uitl::ReadFile(full_path, &content)) {
        resp._status = 500;
        BuildErrorResponse(resp);
        return;
    }

    resp._status = 200;
    resp.SetHeader("Content-Type", Uitl::ExMime(full_path));
    resp.SetHeader("Content-Length", std::to_string(content.size()));
    if (req._method == "GET") {
        resp._body = std::move(content);
    } else {
        resp._body.clear();
    }
}

    void BuildErrorResponse(HttpResponse& resp) {
        std::string status_desc = Uitl::StatuDesc(resp._status);
        std::string body = "<html><head><title>Error</title></head><body>"
                           "<h1>" + std::to_string(resp._status) + " " + status_desc + "</h1>"
                           "</body></html>";
        resp.SetHeader("Content-Type", "text/html");
        resp.SetHeader("Content-Length", std::to_string(body.size()));
        resp._body = std::move(body);
    }

    void SendResponse(const ConnPtr& conn, const HttpResponse& resp) {
        std::string resp_str = SerializeResponse(resp);
        conn->Send(resp_str.c_str(), resp_str.size());
    }

    std::string SerializeResponse(const HttpResponse& resp) {
        std::string ret;
        ret += "HTTP/1.1 " + std::to_string(resp._status) + " " + Uitl::StatuDesc(resp._status) + "\r\n";
        for (auto& [key, value] : resp._headers) {
            ret += key + ": " + value + "\r\n";
        }
        ret += "\r\n";
        ret += resp._body;
        return ret;
    }
};

int main() {
    HttpServer server(1315, "/home/ubuntu/galgamesource/www.ymgal.games/");
    server.Start(4);
    return 0;
}