#include "httpd.h"

#include <string.h> // strpbrk
#include <stdarg.h> // va_start()
#include <unordered_set>
#include <sys/types.h> // stat()
#include <sys/stat.h> // stat()
#include <unistd.h> // stat() getcwd()
#include <sstream>
#include <fcntl.h> // open

namespace miniduo {

std::unordered_map<int, std::pair<std::string, std::string>> responseStatus
{
    {200, {"OK", ""}},
    {400, {"Bad Request", "Your request has bad syntax or is inherently impossible to staisfy.\n"}},
    {403, {"Forbidden", "You do not have permission to get file form this server.\n"}},
    {404, {"Not Found", "The requested file was not found on this server.\n"}},
    {500, {"Internal Error", "There was an unusual problem serving the request file.\n"}}
};

std::unordered_map<std::string, HttpRequest::METHOD> supportedMethods 
{
    {"GET", HttpRequest::METHOD::GET},
    {"get", HttpRequest::METHOD::GET},
    {"POST", HttpRequest::METHOD::POST},
    {"post", HttpRequest::METHOD::POST}
};

} // namespace miniduo

using namespace miniduo;


LINE_STATUS HttpRequest::parseLine(Buffer *buf){
    const char* p = buf->beginRead();
    int len = buf->readableBytes();
    while(p < buf->beginWrite()) {
        if( *p == '\r') {
            if(p+1 == buf->beginWrite()) {
                return LINE_STATUS::LINE_OPEN;
            }
            else if(*(p+1) == '\n') {
                return LINE_STATUS::LINE_OK;
            }
            
            return LINE_STATUS::LINE_BAD;
        }
        else if(*p == '\n') {
            if(p > buf->beginRead() && *(p-1) == '\r') {
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
        p++;
    }
    return LINE_STATUS::LINE_OPEN;
}

HTTP_CODE HttpRequest::parseRequstline(Buffer *buf) {
    std::string line = retrieveLine(buf);
    http_log("HttpRequest::parseRequestline(): %s", line.c_str());
    std::stringstream ss(line);
    std::string method, URL, version;
    ss >> method >> URL >> version;
    http_log("httpRequest line: method[%s], URL[%s], version[%s]", method.c_str(), URL.c_str(), version.c_str());
    if(supportedMethods.find(method) == supportedMethods.end()) {
        return HTTP_CODE::BAD_REQUEST;
    }
    method_ = method;
    methodType_ = supportedMethods[method];
    URL_ = URL;
    version_ = version;
    if(version_ != "HTTP/1.1") {
        /// FIXME:
        // return HTTP_CODE::BAD_REQUEST;
    }
    if(URL_.find("http://") == 0) {
        URL_ = "/" +  URL_.substr(7, URL_.size() - 7);
        http_log("URL without 'http://' : %s", URL_.c_str());
    }
    if(URL_.size() == 0 || URL_[0] != '/') {
        return HTTP_CODE::BAD_REQUEST;
    }
    checkstate_ = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;

}

HTTP_CODE HttpRequest::parseHeaders(Buffer *buf) {
    std::string line = retrieveLine(buf);
    http_log("HttpRequest::parseHeaders(): %s", line.c_str());
    if(line.empty()) {
        if(contentLength_ != 0) {
            checkstate_ = CHECK_STATE::CHECK_STATE_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        checkstate_ = CHECK_STATE::CHECK_STATE_REQUESTLINE;
        return HTTP_CODE::GET_REQUEST;
    }
    else if(line.find("Connection:") == 0) {

    }
    else if(line.find("Content-length:") == 0) {
        contentLength_ = stoi(line.substr(16, line.size()-16));
    }
    else {
        /// FIXME: 替换为 log
        http_log("Unkown header");
        // printf("Unknow header: %s", line.c_str());
    }
    header_.append(line + "\r\n");
    return HTTP_CODE::NO_REQUEST;
}

HTTP_CODE HttpRequest::parseContent(Buffer *buf) {
    std::string line = retrieveLength(buf, contentLength_);
    http_log("HttpRequest::parseContent(): %s", line.c_str());
    contentLength_ -= line.size();
    body_ += line;
    if(contentLength_ == 0) {
        checkstate_ = CHECK_STATE::CHECK_STATE_REQUESTLINE;
        return HTTP_CODE::GET_REQUEST;
    }
    return HTTP_CODE::NO_REQUEST;
}

std::string HttpRequest::retrieveLine(Buffer *buf) {
    const char *p = buf->beginRead();
    std::string line;
    while(p < buf->beginWrite() && !(*p == '\r' && *(p+1) == '\n')) {
        line.push_back(*p++);
    }
    buf->retrieveUntil(p+2); // +2 去掉换行符
    return line;
}

std::string HttpRequest::retrieveLength(Buffer *buf, int len) {
    const char *p = buf->beginRead();
    std::string line;
    while(len-->0 && p < buf->beginWrite()) {
        line.push_back(*p++);
    }
    buf->retrieve(line.size());
    return line;
}

HTTP_CODE HttpRequest::tryDecode(Buffer *buf) {
    LINE_STATUS linestatus = LINE_STATUS::LINE_OK;
    HTTP_CODE retcode = HTTP_CODE::NO_REQUEST;
    while( (checkstate_ == CHECK_STATE::CHECK_STATE_CONTENT && linestatus == LINE_STATUS::LINE_OK) 
        || ((linestatus = parseLine(buf)) == LINE_STATUS::LINE_OK)) 
    {
        switch (checkstate_)
        {
            case CHECK_STATE::CHECK_STATE_REQUESTLINE :
            {
                retcode = parseRequstline(buf);
                if(retcode == HTTP_CODE::BAD_REQUEST) {
                    return HTTP_CODE::BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE::CHECK_STATE_HEADER:
            {
                retcode = parseHeaders(buf);
                if(retcode == HTTP_CODE::BAD_REQUEST) {
                    return HTTP_CODE::BAD_REQUEST;
                }
                else if(retcode == HTTP_CODE::GET_REQUEST) {
                    return HTTP_CODE::GET_REQUEST;
                }
                break;
            }
            case CHECK_STATE::CHECK_STATE_CONTENT:
            {
                retcode = parseContent(buf);
                if( retcode == HTTP_CODE::GET_REQUEST) {
                    return HTTP_CODE::GET_REQUEST;
                }
                linestatus = LINE_STATUS::LINE_OPEN;
                break;
            }
            default:
            {
                return HTTP_CODE::INTERNAL_ERROR;
                break;
            }
        }
    }
    if(linestatus == LINE_STATUS::LINE_OPEN) {
        return HTTP_CODE::NO_REQUEST;
    }
    else {
        return HTTP_CODE::BAD_REQUEST;
    }
}


bool HttpResponse::headersAppend(const std::string &line) {
    header_.append(line);
    return true;
}

bool HttpResponse::headersAppend(const char *format, ...) {
    char buf[2048];
    va_list argList;
    va_start(argList, format);
    vsnprintf(buf, sizeof(buf), format, argList);
    va_end(argList);
    return headersAppend(std::string(buf));
}

bool HttpResponse::addStatusLine(int status) {
    return headersAppend("HTTP/1.1 %d %s\r\n", 
                          status, 
                          responseStatus[status].first.c_str());
}

bool HttpResponse::addHeaders(int len) {
    return addContentLength(len) && addBlankLine();
}

bool HttpResponse::addContentLength(int len) {
    return headersAppend("Content-Length:%d\r\n", len);
}

bool HttpResponse::addContentType() {
    return headersAppend("Content-Type:%s\r\n", "text/html");
}

bool HttpResponse::addBlankLine() {
    return headersAppend("\r\n");
}

bool HttpResponse::addBody(const std::string &content) {
    body_.append(content);
    return true;
}








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
    
}

void HttpServer::onState(const TcpConnectionPtr &conn) {
    if(conn->connected()) {
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

HTTP_CODE HttpServer::decodeHttpRequest(const TcpConnectionPtr &conn, Buffer *buf) {
    http_log("HttpServer::decodeHttpRequest()");
    HttpRequest &req = getHttpRequest(conn);
    return req.tryDecode(buf);
}

/// 执行static请求的检查，以及cgi请求的执行
HTTP_CODE HttpServer::handleRequest(const TcpConnectionPtr &conn) {
    http_log("HttpServer::handleRequest()");
    HttpRequest &req = getHttpRequest(conn);
    HttpResponse &resp = getHttpResponse(conn);
    http_log("method: [%s]\n", req.method_.c_str());
    http_log("URL: [%s]\n", req.URL_.c_str());
    http_log("version: [%s]", req.version_.c_str());
    // 不支持 get cgi
    if(req.methodType_ == HttpRequest::METHOD::GET
        && req.URL_.find('?') < req.URL_.size()) 
    {
        return HTTP_CODE::BAD_REQUEST;
    }
    
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
        resp.addHeaders(responseStatus[status].second.size());
        resp.addBody(responseStatus[status].second);
        break;
    }
    case HTTP_CODE::BAD_REQUEST:
    {
        int status = 400;
        resp.addStatusLine(status);
        http_log("status");
        resp.addHeaders(responseStatus[status].second.size());
        http_log("header");
        resp.addBody(responseStatus[status].second);
        http_log("body");
        break;
    }
    case HTTP_CODE::FORBIDDEN_REQUEST:
    {   
        int status = 403;
        resp.addStatusLine(status);
        resp.addHeaders(responseStatus[status].second.size());
        resp.addBody(responseStatus[status].second);
        break;
    }
    case HTTP_CODE::NO_RESOURCE:
    {
        int status = 404;
        resp.addStatusLine(status);
        resp.addHeaders(responseStatus[status].second.size());
        resp.addBody(responseStatus[status].second);
        break;
    }
    case HTTP_CODE::FILE_REQUEST:
    {
        // 校验 method
        // 校验 uri
        // 打开并获取文件信息
        // 装载 HttpResponse
        resp.addStatusLine(200);
        resp.addHeaders(resp.filesize_);
        int fd = ::open(resp.filepath_.c_str(), O_RDONLY);
        char buf[100];
        int n = 0;
        while((n = read(fd, buf, sizeof(buf))) != 0) {
            // resp.addBody(buf);
            resp.body_.append(buf, n);
        }
        break;
    }
    default:
        break;
    }
}

void HttpServer::sendResponse(const TcpConnectionPtr &conn) {
    http_log("HttpServer::sendReponse()");
    HttpResponse &resp = getHttpResponse(conn);
    conn->send(resp.header_);
    conn->send(resp.body_);
    resp.clear();
}



