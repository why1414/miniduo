#pragma once

#include "buffer.h"
#include "conn.h"
#include "net.h"
#include "callbacks.h"
#include "logging.h"

#include <string>
#include <functional>
#include <unordered_map>


namespace miniduo {

#define http_log(...) log_trace("[http] " __VA_ARGS__)

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
    bool addStatusLine(int status);
    bool addHeaders(int len);
    bool addContentLength(int len);
    bool addContentType() ;
    bool addBlankLine();
    bool addBody(const std::string &content) ;

// private:
    bool headersAppend(const std::string &line) ;
    bool headersAppend(const char *format, ...);
    // STATUS status_;
    std::string server_;
    int contentLen_;
    std::string filepath_;
    int filesize_;
}; // class HttpResponse

class HttpServer {
public:
    HttpServer(EventLoop *loop, const SockAddr& listenAddr);

    typedef std::function<void (const TcpConnectionPtr &)> HttpCallback;
    // void setRequest(const std::string &method, const HttpCallback &cb) { cbs_[method] = cb; }
    // void setDefault(const HttpCallback &cb) { defcb_ = cb; }
    void start() { tcpServer_.start(); }

private:
    void onState(const TcpConnectionPtr &conn);
    void onMsg(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp recvTime );
    HTTP_CODE decodeHttpRequest(const TcpConnectionPtr &conn, Buffer *buf);
    HTTP_CODE handleRequest(const TcpConnectionPtr &conn);
    void loadResponse(const TcpConnectionPtr &conn, HTTP_CODE retcode);
    void sendResponse(const TcpConnectionPtr &conn);
    HttpRequest& getHttpRequest(const TcpConnectionPtr &conn) 
    { return conn->getContext<HttpContext>().req; }
    HttpResponse& getHttpResponse(const TcpConnectionPtr &conn)
    { return conn->getContext<HttpContext>().resp; }
    struct HttpContext {
        HttpRequest req;
        HttpResponse resp;
    };

    // HttpCallback defcb_;
    TcpServer tcpServer_;
    // std::map<std::string, HttpCallback> cbs_;
    // 资源目录相对路径(相对于工作路径)
    const std::string resourcePath_ = "../resource";

}; // class HttpServer


} // namespace miniduo