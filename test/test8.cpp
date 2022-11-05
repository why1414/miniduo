#include "EventLoop.h"
#include "callbacks.h"
#include "conn.h"

#include <unistd.h>

using namespace miniduo;

void onConnection(const TcpConnectionPtr& conn) {
    if(conn->connected()) {
        printf("onConnection(): new connection [%s] from %s\n",
        conn->name().c_str(),
        conn->peerAddress().addrString().c_str() );
    }
    else {
        printf("onConnection(): connection [%s] is down\n",
                conn->name().c_str());
    }
}

void onMsg(const TcpConnectionPtr& conn,
           const char* data,
           ssize_t len) 
{
    printf("onMsg(): received %zd bytes from connection [%s]\n",
           len, conn->name().c_str());
}

int main() {
    printf("main(): pid = %d\n", getpid());
    SockAddr listenAddr(9981);
    EventLoop loop;

    TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onConnection);
    server.setMsgCallback(onMsg);
    server.start();

    loop.loop();
}