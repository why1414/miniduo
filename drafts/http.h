#pragma once

#include "buffer.h"
#include "conn.h"
#include "net.h"
#include "callbacks.h"

#include <string>
#include <functional>
#include <unordered_map>

namespace miniduo {


// 主状态机：分析请求行，分析头部字段
enum class CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
};

enum class LINE_STATUS {
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN
};

enum class HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

std::unordered_map<int, std::pair<std::string, std::string>> responseStatus
{
    {200, {"OK", ""}},
    {400, {"Bad Request", "Your request has bad syntax or is inherently impossible to staisfy.\n"}},
    {403, {"Forbidden", "You do not have permission to get file form this server.\n"}},
    {404, {"Not Found", "The requested file was not found on this server.\n"}},
    {500, {"Internal Error", "There was an unusual problem serving the request file.\n"}}
};





struct HttpMsg {
    std::string header_;
    std::string body_;
    void clear() { 
        header_.clear();
        body_.clear();
    }
};

class HttpRequest: public HttpMsg {

public:
    HttpRequest() {}
    ~HttpRequest() {}
    enum class METHOD {
        GET = 0,
        HEAD,
        POST,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    // 从buffer中解析 http request
    HTTP_CODE tryDecode(Buffer *buf);

// private:
    LINE_STATUS parseLine(Buffer* buf);
    HTTP_CODE parseHeaders(Buffer *buf);
    HTTP_CODE parseRequstline(Buffer *buf);
    HTTP_CODE parseContent(Buffer *buf);
    std::string retrieveLine(Buffer *buf);
    // try to retrieve at most len characters
    std::string retrieveLength(Buffer *buf, int len);

    CHECK_STATE checkstate_ = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    METHOD methodType_;
    std::string method_;
    std::string URL_;
    std::string version_;
    int contentLength_ = 0;
    std::string args_;
    
};

class HttpResponse: public HttpMsg {
public:
    HttpResponse() {}
    ~HttpResponse() {}
    std::string encode();
    void setStatus();
    void setBody();
    bool addStatusLine(int status) { return headersAppend("HTTP/1.1 %d %s\r\n", status, responseStatus[status].first.c_str()); }
    bool addHeaders(int len) { return addContentLength(len) && addBlankLine(); }
    bool addContentLength(int len) { return headersAppend("Content-Length:%d\r\n", len); };
    bool addContentType() { return headersAppend("Content-Type:%s\r\n", "text/html"); }
    bool addBlankLine() { return headersAppend("\r\n"); }
    bool addBody(const std::string &content) { body_.append(content); return true; }

private:
    bool headersAppend(const std::string &line) ;
    bool headersAppend(const char *format, ...);
    // STATUS status_;
    std::string server_;
    int contentLen_;


}; // class HttpResponse


class HttpServer {

    HttpServer(EventLoop *loop, const SockAddr& listenAddr);

    typedef std::function<void (const TcpConnectionPtr &)> HttpCallback;
    void setRequest(const std::string &method, const HttpCallback &cb) { cbs_[method] = cb; }
    void setDefault(const HttpCallback &cb) { defcb_ = cb; }
    void start() { tcpServer_.start(); }

private:
    void onState(const TcpConnectionPtr &conn);
    void onMsg(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp recvTime );
    HTTP_CODE handleRequest(const TcpConnectionPtr &conn);
    void loadResponse(const TcpConnectionPtr &conn, HTTP_CODE retcode);

    HttpRequest& getHttpRequest(const TcpConnectionPtr &conn) 
    { return conn->getContext<HttpContext>().req; }
    HttpResponse& getHttpResponse(const TcpConnectionPtr &conn)
    { return conn->getContext<HttpContext>().resp; }
    struct HttpContext {
        HttpRequest req;
        HttpResponse resp;
    };

    HttpCallback defcb_;
    TcpServer tcpServer_;
    std::map<std::string, HttpCallback> cbs_;

}; // class HttpServer


} // namespace miniduo

