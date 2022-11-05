#include "conn.h"
#include "EventLoop.h"
#include "net.h"

#include <unistd.h>
#include <thread>

void newConnection(int sockfd, const miniduo::SockAddr& peerAddr) {
    printf("newConnection(): accepted a new connection from %s\n", peerAddr.addrString().c_str());
    write(sockfd, "how are you?\n", 13);
    close(sockfd);
}

int main() {\
    printf("main(): pid = %d\n", getpid());
    miniduo::SockAddr listenAddr(9981);
    miniduo::EventLoop loop, loop2;

    miniduo::Acceptor acceptor(&loop2, listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();
    // std::thread t([&] { loop2.loop();});
    loop2.runAfter(3, [] {printf("run after 3s\n");});
    loop2.loop();
    
}