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
    int port = 9981;
    if(argc > 2) {
        printf("./echosvr [port]\n");
        exit(1);
    }
    else if(argc == 2) {
        port = atoi(argv[1]);
    }
    EventLoop loop;
    SockAddr listenAddr(port);
    TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onState);
    server.setMsgCallback(onMsg);
    server.start();
    loop.loop();
    return 0;
}