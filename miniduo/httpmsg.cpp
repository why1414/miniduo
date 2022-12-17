#include "httpmsg.h"

#include <string.h> // strpbrk
#include <stdarg.h> // va_start()
#include <unordered_set>
#include <sys/types.h> // stat()
#include <sys/stat.h> // stat()
#include <unistd.h> // stat() 
#include <sstream>
#include <fcntl.h> // open

namespace miniduo {

std::unordered_map<int, std::pair<std::string, std::string>> HttpResponse::responseStatus
{
    {200, {"OK", ""}},
    {400, {"Bad Request", "Your request has bad syntax or is inherently impossible to staisfy.\n"}},
    {403, {"Forbidden", "You do not have permission to get file form this server.\n"}},
    {404, {"Not Found", "The requested file was not found on this server.\n"}},
    {500, {"Internal Error", "There was an unusual problem serving the request file.\n"}}
};

std::unordered_map<std::string, METHOD> HttpRequest::supportedMethods 
{
    {"GET", METHOD::GET},
    {"POST", METHOD::POST}
};

std::unordered_map<std::string, VERSION> HttpRequest::supportedVersions
{
    {"HTTP/1.0", VERSION::HTTP10},
    {"HTTP/1.1", VERSION::HTTP11},
    {"http/1.0", VERSION::HTTP10},
    {"http/1.1", VERSION::HTTP11}
};

} // namespace miniduo

using namespace miniduo;


LINE_STATUS HttpRequest::parseLine(Buffer *buf) {
    http_log("step in this call");
    if(buf->findCRLF() != nullptr ) {
        return LINE_STATUS::LINE_OK;
    }
    else if(::memchr(buf->beginRead(), '\n', buf->readableBytes()) != nullptr ){
        return LINE_STATUS::LINE_BAD;
    }
    else {
        return LINE_STATUS::LINE_OPEN;
    }
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
    // 去除 http:// 字段
    if(URL.find("http://") == 0) {
        // http_log("remove 'http://'");
        URL = URL.substr(7, URL.size() - 7);
    }
    // 去除 域名字段
    std::size_t idx = 0;
    if((idx = URL.find('/')) != std::string::npos) {
        // http_log("remove yuming");
        URL = URL.substr(idx, URL.size() - idx);
    }
    else {
        return HTTP_CODE::BAD_REQUEST;
    }
    if((idx = URL.find('?')) != std::string::npos) {
        URL_ = URL.substr(0, idx);
        idx++;
        query_ = URL.substr(idx, URL.size() - idx);
    }
    else {
        URL_ = URL;
    }
    
    if(supportedVersions.find(version) == supportedVersions.end()) {
        return HTTP_CODE::BAD_REQUEST;
    }
    version_ = version;
    versionType_ = supportedVersions[version];

    checkstate_ = CHECK_STATE::EXPECT_HEADER;
    // http_log("end of this call");
    return HTTP_CODE::NO_REQUEST;
    

}

HTTP_CODE HttpRequest::parseHeaders(Buffer *buf) {
    http_log("step in this call");
    std::string line = retrieveLine(buf);
    http_log("HttpRequest::parseHeaders(): %s", line.c_str());
    std::size_t idx = 0;
    if(line.empty()) {
        if(headers_.find("Content-length") != headers_.end() 
           && contentLength_ > 0) 
        {
            checkstate_ = CHECK_STATE::EXPECT_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        checkstate_ = CHECK_STATE::GET_ALL;
        return HTTP_CODE::GET_REQUEST;
    }
    else if((idx = line.find(':')) != std::string::npos) {
        std::string field = line.substr(0, idx);
        idx += 2; // remove ':' & ' '
        std::string value = line.substr(idx, line.size() - idx);
        headers_[field] = value;
        if(field == "Content-length") {
            contentLength_ = std::stoi(value);
        }
    }
    else {
        http_log("Unkown header");
    }
    return HTTP_CODE::NO_REQUEST;
}

HTTP_CODE HttpRequest::parseContent(Buffer *buf) {
    if(buf->readableBytes() < contentLength_) {
        return HTTP_CODE::NO_REQUEST;
    }
    content_ = std::string(buf->beginRead(), contentLength_);
    buf->retrieve(contentLength_);
    checkstate_ = CHECK_STATE::GET_ALL;
    return HTTP_CODE::GET_REQUEST;
}

std::string HttpRequest::retrieveLine(Buffer *buf) {
    const char* crlf = buf->findCRLF();
    http_log("begin:%p, crlf:%p",buf->beginRead(), crlf);
    assert(crlf != nullptr);
    http_log("%d", crlf-buf->beginRead());
    std::string line(buf->beginRead(), crlf-buf->beginRead());
    http_log("%d", crlf-buf->beginRead());
    buf->retrieveUntil(crlf+2);
    return line;
}


HTTP_CODE HttpRequest::tryDecode(Buffer *buf) {
    LINE_STATUS linestatus = LINE_STATUS::LINE_OK;
    HTTP_CODE retcode = HTTP_CODE::NO_REQUEST;
    while( (checkstate_ != CHECK_STATE::GET_ALL) 
            &&
           (checkstate_ == CHECK_STATE::EXPECT_CONTENT || (linestatus = parseLine(buf)) == LINE_STATUS::LINE_OK)) 
    {
        switch (checkstate_)
        {
            case CHECK_STATE::EXPECT_REQUESTLINE :
            {
                retcode = parseRequstline(buf);
                if(retcode == HTTP_CODE::BAD_REQUEST) {
                    return HTTP_CODE::BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE::EXPECT_HEADER:
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
            case CHECK_STATE::EXPECT_CONTENT:
            {
                retcode = parseContent(buf);
                if( retcode == HTTP_CODE::GET_REQUEST) {
                    return HTTP_CODE::GET_REQUEST;
                }
                return HTTP_CODE::NO_REQUEST;
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
    headers_.append(line);
    return true;
}

bool HttpResponse::headersAppend(const char *format, ...) {
    char buf[2048] = {0};
    va_list argList;
    va_start(argList, format);
    vsnprintf(buf, sizeof(buf), format, argList);
    va_end(argList);
    return headersAppend(std::string(buf, sizeof(buf)));
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

