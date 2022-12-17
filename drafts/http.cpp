#include "http.h"

#include <string.h> // strpbrk
#include <stdarg.h> // va_start()
#include <unordered_set>

namespace miniduo {

std::unordered_map<std::string, HttpRequest::METHOD> supportedMethods 
{
    {"GET", HttpRequest::METHOD::GET},
    {"POST", HttpRequest::METHOD::POST}
};

} // namespace miniduo;

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
    int endOfmethod = line.find(' ');
    if(endOfmethod >= line.size()) {
        return HTTP_CODE::BAD_REQUEST;
    }
    std::string method = line.substr(0, endOfmethod);
    // method 检查
    if(supportedMethods.find(method) == supportedMethods.end()) {
        return HTTP_CODE::BAD_REQUEST;
    }
    methodType_ = supportedMethods[method];
    int endOfURL = line.find(' ', endOfmethod+1);
    if(endOfURL >= line.size()) {
        return HTTP_CODE::BAD_REQUEST;
    }
    URL_ = line.substr(endOfmethod+1, endOfURL - endOfmethod);
    version_ = line.substr(endOfURL+1, line.size() - endOfURL - 1);
    if(version_ != "HTTP/1.1") {
        return HTTP_CODE::BAD_REQUEST;
    }
    
    if(URL_.find("http://") == 0) {
        URL_ = "/" +  URL_.substr(7, URL_.size() - 7);
    }
    if(URL_.size() == 0 || URL_[0] != '/') {
        return HTTP_CODE::BAD_REQUEST;
    }
    checkstate_ = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;

}

HTTP_CODE HttpRequest::parseHeaders(Buffer *buf) {
    std::string line = retrieveLine(buf);
    if(line.empty()) {
        if(contentLength_ != 0) {
            checkstate_ = CHECK_STATE::CHECK_STATE_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        return HTTP_CODE::GET_REQUEST;
    }
    else if(line.find("Connection:") == 0) {

    }
    else if(line.find("Content-length:") == 0) {
        contentLength_ = stoi(line.substr(17, line.size()-17));
    }
    else {
        /// FIXME: 替换为 log
        printf("Unknow header: %s", line.c_str());
    }
    header_.append(line + "\r\n");
    return HTTP_CODE::NO_REQUEST;
}

HTTP_CODE HttpRequest::parseContent(Buffer *buf) {
    std::string line = retrieveLength(buf, contentLength_);
    contentLength_ -= line.size();
    body_ += line;
    if(contentLength_ == 0) {
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
    char buf[1024];
    va_list argList;
    va_start(argList, format);
    vsnprintf(buf, 1024, format, argList);
    va_end(argList);
    return headersAppend(buf);
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

void HttpServer::onMsg(const TcpConnectionPtr &conn,
                        Buffer *buf,
                        Timestamp recvTime) 
{
    HttpRequest &req = getHttpRequest(conn);
    auto retcode = req.tryDecode(buf);
    if(retcode == HTTP_CODE::NO_REQUEST) {
        // more data needed
        printf("HtttpServer::OnMsg(): more data needed\n");
        return;
    }
    if(retcode == HTTP_CODE::GET_REQUEST) {
        retcode = handleRequest(conn);
    }
    loadResponse(conn, retcode);
    // sendResponse(const TcpConnectionPtr& conn);
    // 
} 

HTTP_CODE HttpServer::handleRequest(const TcpConnectionPtr &conn){
    HttpRequest &req = getHttpRequest(conn);
    
    // check and parse url

    //
}

void HttpServer::loadResponse(const TcpConnectionPtr &conn, HTTP_CODE retcode) {
    HttpResponse &resp = getHttpResponse(conn);
    switch (retcode)
    {
    case HTTP_CODE::INTERNAL_ERROR:
    {
        resp.addStatusLine(500);
        resp.addHeaders(responseStatus[500].second.size());
        resp.addBody(responseStatus[500].second);
        break;
    }
    case HTTP_CODE::BAD_REQUEST:
    {
        resp.addStatusLine(404);
        resp.addHeaders(responseStatus[404].second.size());
        resp.addBody(responseStatus[404].second);
        break;
    }
    case HTTP_CODE::FORBIDDEN_REQUEST:
    {   
        resp.addStatusLine(403);
        resp.addHeaders(responseStatus[403].second.size());
        resp.addBody(responseStatus[403].second);
        break;
    }
    case HTTP_CODE::FILE_REQUEST:
    {
        // 校验 method
        // 校验 uri
        // 打开并获取文件信息
        // 装载 HttpResponse
        resp.addStatusLine(200);
        break;
    }
    default:
        break;
    }
}
