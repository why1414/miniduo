#include "net.h"
#include "logging.h"

#include <arpa/inet.h> // inet_ntop()
#include <cstring>  // bzero()
#include <sys/types.h>    // socket(2)
#include <sys/socket.h>    // socket(2)
#include <cassert>
#include <unistd.h> // close()


namespace miniduo {
namespace socket {

int createNonblockingSock() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    assert(sockfd >= 0);
    return sockfd;
}

void setReuseAddr(int sockfd, bool value) {
    int optval = value ? 1 : 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                &optval, sizeof(optval));
    assert(ret >= 0);
}

void bindAddr(int sockfd, const SockAddr& addr) {
    int ret = bind(sockfd, (struct sockaddr *) &addr.getSockAddr(), 
                    sizeof(struct sockaddr));
    assert(ret >= 0);

}

void listenSock(int sockfd) {
    int ret = listen(sockfd, 20);
    assert(ret >= 0);
}
/// FIXME: 错误处理， 一次多读
int acceptSock(int sockfd, SockAddr* peerAddr) {
    struct sockaddr_in paddr;
    socklen_t alen = sizeof(paddr);
    bzero(&paddr, sizeof(paddr));
    int connfd = accept(sockfd, (struct sockaddr *) &paddr, &alen );
    // assert(connfd >= 0);
    if(connfd >= 0){
        peerAddr->setSockAddr(paddr);
    }
    return connfd;
}

sockaddr_in getLocalAddr(int sockfd) {
    struct sockaddr_in localAddr;
    socklen_t len = sizeof(localAddr);
    int ret = getsockname(sockfd, (sockaddr *)&localAddr, &len);
    assert(ret >= 0);
    return localAddr;
}

sockaddr_in getPeerAddr(int sockfd) {
    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);
    int ret = getpeername(sockfd, (sockaddr *)&peerAddr, &len);
    assert(ret >= 0);
    return peerAddr;
}

void shutdownWrite(int sockfd) {
    int ret = shutdown(sockfd, SHUT_WR);
    assert(ret >= 0);
}

void close(int sockfd) {
    int ret = ::close(sockfd);
    assert(ret >= 0);
}

int connect(int sockfd, sockaddr_in serverAddr) {
    socklen_t len = sizeof(serverAddr);
    int ret = ::connect(sockfd, (struct sockaddr*) &serverAddr, len);
    return ret;
}

int getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = sizeof(optval);
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}

bool isSelfConnect(int sockfd) {
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port
        && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

} // namespace socket
} // namespace miniduo


using namespace miniduo;

SockAddr::SockAddr(uint16_t port) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = ::htons(port);
}

SockAddr::SockAddr(const std::string& ip, uint16_t port) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    ::inet_pton(AF_INET, ip.data(), &addr_.sin_addr.s_addr);
    addr_.sin_port = htons(port);
}


std::string SockAddr::addrString() const {
    std::string ipstr = ipString();
    uint16_t port = ntohs(addr_.sin_port);
    char buf[32];
    snprintf(buf, 32, "%s:%d", ipstr.data(), port);
    return std::string(buf); 
}

std::string SockAddr::ipString() const {
    char ip[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(addr_.sin_family, &addr_.sin_addr.s_addr, ip, sizeof(ip));
    return std::string(ip);
}

uint32_t SockAddr::ipInt() const {
    return ntohl(addr_.sin_addr.s_addr);
}

uint16_t SockAddr::port() const {
    return ntohs(addr_.sin_port);
}

