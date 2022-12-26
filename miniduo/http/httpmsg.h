#pragma once

#include "miniduo/buffer.h"
#include "miniduo/conn.h"
#include "miniduo/net.h"
#include "miniduo/callbacks.h"
#include "miniduo/logging.h"

#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <string.h> // memchr()
#include <unistd.h> // close()


namespace miniduo {

#define http_log(...) log_trace("[http] " __VA_ARGS__)



// 主状态机：分析请求行，分析头部字段
enum class CHECK_STATE {
    EXPECT_REQUESTLINE = 0,
    EXPECT_HEADER,
    EXPECT_CONTENT,
    GET_ALL
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

enum class METHOD {
    INVALID = 0,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATCH,
    
};

enum class VERSION {
    INVALID,
    HTTP10,
    HTTP11,
};


struct HttpRequest {
    HttpRequest() {}
    ~HttpRequest() {}

    void clear() {
        checkstate_ = CHECK_STATE::EXPECT_REQUESTLINE;
        methodType_ = METHOD::INVALID;
        method_.clear();
        URL_.clear();
        query_.clear();
        versionType_ = VERSION::INVALID;
        version_.clear();
        headers_.clear();
        contentLength_ = 0;
        content_.clear();

    }
    // 从buffer中解析 http request
    HTTP_CODE tryDecode(Buffer *buf);
    LINE_STATUS parseLine(Buffer* buf);
    HTTP_CODE parseHeaders(Buffer *buf);
    HTTP_CODE parseRequstline(Buffer *buf);
    HTTP_CODE parseContent(Buffer *buf);
    std::string retrieveLine(Buffer *buf);
    // try to retrieve at most len characters
    // std::string retrieveLength(Buffer *buf, int len);

    CHECK_STATE checkstate_ = CHECK_STATE::EXPECT_REQUESTLINE;

    METHOD methodType_ = METHOD::INVALID;
    std::string method_;
    std::string URL_;
    std::string query_;
    VERSION versionType_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    int contentLength_ = 0;
    std::string content_;

    static std::unordered_map<std::string, METHOD> supportedMethods ;
    static std::unordered_map<std::string, VERSION> supportedVersions;
};

struct HttpResponse {

    HttpResponse() {}
    ~HttpResponse() {}
    void clear() {
        statusCode_ = 0;
        statusStr_.clear();
        headers_.clear();
        body_.clear();
        closeConnection_ = true;
        respComplete_ = false;
        if(filefd_ != -1) {
            ::close(filefd_);
            filefd_ = -1;
        }
    }

    bool addStatusLine(int status);
    bool addHeaders(int len);
    bool addContentLength(int len);
    bool addContentType() ;
    bool addBlankLine();
    bool addBody(const std::string &content) ;
    bool addBody(const char* buf, size_t len) {
        body_.append(buf, len);
        return true;
    }

    bool headersAppend(const std::string &line) ;
    bool headersAppend(const char *format, ...);
    // STATUS status_;
    int statusCode_;
    std::string statusStr_;
    std::string headers_;
    std::string body_;

    bool closeConnection_ = true;
    bool respComplete_ = false;

    int filefd_ = -1;

    static std::unordered_map<int, std::pair<std::string, std::string>> responseStatus;
}; // class HttpResponse



struct HttpContext {
    HttpRequest req;
    HttpResponse resp;
};

} // namespace miniduo