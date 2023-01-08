#pragma once

#include "miniduo/buffer.h"
#include "miniduo/conn.h"
#include "miniduo/net.h"
#include "miniduo/callbacks.h"
#include "httpmsg.h"

#include <string>
#include <functional>
#include <unordered_map>


namespace miniduo {

// extern std::unordered_map<int, std::pair<std::string, std::string>> responseStatus;
// extern std::unordered_map<std::string, METHOD> supportedMethods ;
// extern std::unordered_map<std::string, VERSION> supportedVersions;

class HttpServer {
public:
    HttpServer(EventLoop *loop, const SockAddr& listenAddr);

    // typedef std::function<void (const TcpConnectionPtr &)> HttpCallback;
    void start() { tcpServer_.start(); }
    void setResourcePath(std::string &path) {
        resourcePath_ = path;
    }

private:
    void onState(const TcpConnectionPtr &conn);
    void onMsg(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp recvTime );
    void onWriteComplete(const TcpConnectionPtr &conn);

    HTTP_CODE decodeHttpRequest(const TcpConnectionPtr &conn, Buffer *buf);
    HTTP_CODE handleRequest(const TcpConnectionPtr &conn);
    void loadResponse(const TcpConnectionPtr &conn, HTTP_CODE retcode);
    void sendResponse(const TcpConnectionPtr &conn);
    void loadFailResponse(HttpResponse& resp, int status);

    HttpRequest& getHttpRequest(const TcpConnectionPtr &conn) 
    { return conn->getContext<HttpContext>().req; }
    HttpResponse& getHttpResponse(const TcpConnectionPtr &conn)
    { return conn->getContext<HttpContext>().resp; }

    TcpServer tcpServer_;

    std::string resourcePath_ ;

}; // class HttpServer


} // namespace miniduo