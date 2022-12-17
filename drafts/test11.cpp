#include "tcpclient.h"
#include "EventLoop.h"
#include "net.h"

using namespace miniduo;

EventLoop* g_loop;

void connectCallback(int sockfd) {
    printf("connected.\n");
    g_loop->quit();
}

int main(int argc, char* argv[]) {
    EventLoop loop;
    g_loop = &loop;
    SockAddr serverAddr("127.0.0.1", 9981);
    ConnectorPtr connector(new Connector(&loop, serverAddr));
    connector->setNewConnectionCallback(connectCallback);
    connector->start();
    loop.loop();
}