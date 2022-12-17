#include <miniduo/miniduo.h>

using namespace miniduo;

void onState(const TcpConnectionPtr &conn) {
    if(conn->connected()) {
        log_trace("new connection");
    }
}

void onMsg(const TcpConnectionPtr &conn,
            Buffer *buf,
            Timestamp recvTime) 
{
    conn->send(buf->retrieveAsString());
}

int main(int argc, const char *argv[]) {
    set_logLevel(Logger::LogLevel::DISABLE);
    EventLoops loop(17);
    SockAddr echolisten(8888);
    SockAddr httplisten(9999);
    TcpServer server(&loop, echolisten);
    server.setConnectionCallback(onState);
    server.setMsgCallback(onMsg);
    server.start();
    HttpServer httpserver(&loop, httplisten);
    std::string resPath = "/mnt/e/GitHub/miniduo_/resource";
    httpserver.setResourcePath(resPath);
    httpserver.start();
    loop.loop();
    return 0;
}