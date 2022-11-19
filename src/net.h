#pragma once

#include <string>
#include <netinet/in.h>

namespace miniduo
{

class SockAddr;

namespace socket {

extern int createNonblockingSock();
extern void setReuseAddr(int sockfd, bool value = true); // ???
extern void bindAddr(int sockfd, const SockAddr& addr);
extern void listenSock(int sockfd);
extern int acceptSock(int sockfd, SockAddr* peerAddr);
extern sockaddr_in getLocalAddr(int sockfd);
extern sockaddr_in getPeerAddr(int sockfd);
extern void shutdownWrite(int sockfd);
} // namespace socket

class SockAddr {
public:
    /// @brief 构造一个listening的sock explicit
    explicit SockAddr(uint16_t port);
    SockAddr(const std::string& ip, uint16_t port);
    SockAddr(const struct sockaddr_in& addr)
        : addr_(addr)
    {}

    std::string addrString() const;
    std::string ipString() const;
    uint16_t port() const;
    uint32_t ipInt() const;

    const struct sockaddr_in& getSockAddr() const {
        return addr_;
    }
    void setSockAddr(const struct sockaddr_in& addr) {
        addr_ = addr;
    }


private:
    struct sockaddr_in addr_;
}; // class sockAddr



} // namespace miniduo
    