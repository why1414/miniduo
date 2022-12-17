#include "httpd.h"

#include <string.h> // strpbrk
#include <stdarg.h> // va_start()
#include <unordered_set>
#include <sys/types.h> // stat()
#include <sys/stat.h> // stat()
#include <unistd.h> // stat() getcwd()
#include <sstream>
#include <fcntl.h> // open

using namespace miniduo;


HttpServer::HttpServer(EventLoop *loop, const SockAddr &listenAddr) 
    : tcpServer_(loop, listenAddr)
{
    // onCreate()
    tcpServer_.setConnectionCallback( 
        [this] (const TcpConnectionPtr &conn) -> void {
            onState(conn);
        }
    );
    // onMsg();
    tcpServer_.setMsgCallback(
        [this] (const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp recvTime) -> void
        {
            onMsg(conn, buf, recvTime);
        }
    );
    // onWriteComplete();
    tcpServer_.setWriteCompleteCallback(
        [this] (const TcpConnectionPtr &conn) ->void {
            onWriteComplete(conn);
        }
    );
    
}

void HttpServer::onState(const TcpConnectionPtr &conn) {
    if(conn->connected()) {
        // 分配 HttpContext
        // conn->getContext<HttpContext>();
        http_log("New conn from [%s]", conn->peerAddress().addrString().c_str());
    }
}

void HttpServer::onMsg(const TcpConnectionPtr &conn,
                        Buffer *buf,
                        Timestamp recvTime) 
{
    http_log("New msg arrived");
    HTTP_CODE retcode = decodeHttpRequest(conn, buf);
    if(retcode == HTTP_CODE::NO_REQUEST) {
        http_log("HTTP_CODE::NO_REQUEST");
        return;
    }
    else if(retcode == HTTP_CODE::GET_REQUEST) {
        http_log("HTTP_CODE::GET_REQUEST");
        retcode = handleRequest(conn);
    }
    loadResponse(conn, retcode);
    sendResponse(conn);
    
    // sendResponse(const TcpConnectionPtr& conn);
    // 
} 

void HttpServer::onWriteComplete(const TcpConnectionPtr &conn) {
    // http_log("more date to send");
    HttpRequest &req = getHttpRequest(conn);
    HttpResponse &resp = getHttpResponse(conn);
    if(req.checkstate_ == CHECK_STATE::GET_ALL && resp.fileopen_ == true) {
        char buf[2048];
        memset(buf, '\0', sizeof(buf));
        int n = ::read(resp.fd_, buf, sizeof(buf));
        if(n > 0) {
            conn->send(std::string(buf, n));
            http_log("%d data has been sent this time", n);
            return ;
        }
        else {
            http_log("File sending completed");
            bool close = resp.closeConnection_;
            req.clear();
            resp.clear();
            if(close == true) {
                conn->closeInNextLoop();
            }
            // conn->closeInNextLoop();
            
            return ;
        }
        
    } 
}

HTTP_CODE HttpServer::decodeHttpRequest(const TcpConnectionPtr &conn, Buffer *buf) {
    http_log("HttpServer::decodeHttpRequest()");
    HttpRequest &req = getHttpRequest(conn);
    if(req.checkstate_ == CHECK_STATE::GET_ALL) {
        return HTTP_CODE::NO_REQUEST;
    }
    return req.tryDecode(buf);
}

/// 执行static请求的检查
HTTP_CODE HttpServer::handleRequest(const TcpConnectionPtr &conn) {
    http_log("HttpServer::handleRequest()");
    HttpRequest &req = getHttpRequest(conn);
    HttpResponse &resp = getHttpResponse(conn);
    http_log("method: [%s]\n", req.method_.c_str());
    http_log("URL: [%s]\n", req.URL_.c_str());
    http_log("version: [%s]", req.version_.c_str());
    
    std::string filePath =  resourcePath_ + req.URL_;
    http_log("file: [%s]", filePath.c_str());
    struct stat fileStat;
    // 文件不存在
    if( stat(filePath.c_str(), &fileStat) < 0) {
        http_log("No such file");
        return HTTP_CODE::NO_RESOURCE;
    }
    // 用户无权限
    if( !(fileStat.st_mode & S_IROTH) ) {
        return HTTP_CODE::FORBIDDEN_REQUEST;
    }
    // 文件夹文件
    if(S_ISDIR(fileStat.st_mode)) {
        return HTTP_CODE::BAD_REQUEST;
    }

    resp.filesize_ = fileStat.st_size;
    resp.filepath_ = filePath;
    http_log("file content length: %d", fileStat.st_size);
    return HTTP_CODE::FILE_REQUEST;
}

void HttpServer::loadResponse(const TcpConnectionPtr &conn, HTTP_CODE retcode) {
    http_log("HttpServer::loadResponse()");
    HttpResponse &resp = getHttpResponse(conn);
    HttpRequest &req = getHttpRequest(conn);
    switch (retcode)
    {
    case HTTP_CODE::INTERNAL_ERROR:
    {
        int status = 500;
        resp.addStatusLine(status);
        resp.addHeaders(resp.responseStatus[status].second.size());
        resp.addBody(resp.responseStatus[status].second);
        break;
    }
    case HTTP_CODE::BAD_REQUEST:
    {
        int status = 400;
        resp.addStatusLine(status);
        resp.addHeaders(resp.responseStatus[status].second.size());
        resp.addBody(resp.responseStatus[status].second);
        break;
    }
    case HTTP_CODE::FORBIDDEN_REQUEST:
    {   
        int status = 403;
        resp.addStatusLine(status);
        resp.addHeaders(resp.responseStatus[status].second.size());
        resp.addBody(resp.responseStatus[status].second);
        break;
    }
    case HTTP_CODE::NO_RESOURCE:
    {
        int status = 404;
        resp.addStatusLine(status);
        resp.addHeaders(resp.responseStatus[status].second.size());
        resp.addBody(resp.responseStatus[status].second);
        break;
    }
    case HTTP_CODE::FILE_REQUEST:
    {
        // while(resp.fd_ < 0) {
        //     resp.fd_ = ::open(resp.filepath_.c_str(), O_RDONLY);
        // }
        resp.fd_ = ::open(resp.filepath_.c_str(), O_RDONLY);
        // printf("%d\n", resp.fd_);
        assert(resp.fd_ > 0);
        
        resp.addStatusLine(200);
        resp.addHeaders(resp.filesize_);
        http_log("open file");
        char buf[2 * 1024]; // 2k bytes
        int n = ::read(resp.fd_, buf, sizeof(buf));
        http_log("read fd");
        if(n <= 0) {
            assert(n != -1);
            http_log("Error read n is less tha 0");
            break;
        }
        resp.addBody(buf, n);
        if( n < resp.filesize_) {
            resp.fileopen_ = true;
            http_log("filesize is larger than buffer");
        }
        else { // n == resp.filesize_
            http_log("filesize is less than buffer");
        }
        
        break;
    }
    default:
        break;
    }
}

void HttpServer::sendResponse(const TcpConnectionPtr &conn) {
    HttpResponse &resp = getHttpResponse(conn);
    HttpRequest &req = getHttpRequest(conn);
    http_log("HttpServer::sendReponse()");
    conn->send(resp.headers_);
    conn->send(resp.body_);
    if(resp.fileopen_ == false) {
        bool close = resp.closeConnection_;
        req.clear();
        resp.clear();
        if(close == true) {
            conn->closeInNextLoop();  
        }
        
    }
    
}



