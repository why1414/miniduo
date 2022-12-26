#pragma once

#include "channel.h"
#include "callbacks.h"
#include "net.h"
#include "buffer.h"
#include "util.h" // AutoContext

#include <functional>
#include <map>

namespace miniduo
{


class EventLoop;


class Acceptor {

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

public:
    typedef std::function<void (int sockfd, const SockAddr&)> NewConnectionCallback;
    
    Acceptor(EventLoop* loop, const SockAddr& listenAddr);
    ~Acceptor() {
        if(acceptFd_ > 0) {
            socket::close(acceptFd_);
        }
    };

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }
    bool listenning() const { return listening_; }
    /// @brief 执行创建TCP服务步骤，调用socket, bind, listen, 出错即终止
    void listen();

private:
    void handleRead(Timestamp recvTime);
    EventLoop* loop_;
    const int acceptFd_;    // listening sockfd
    Channel acceptChannel_; // listening Channel;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;

};


class TcpServer {
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
public:
    TcpServer(EventLoop* loop, const SockAddr& listenAddr);
    ~TcpServer();

    void start();

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMsgCallback(const MsgCallback& cb) {
        msgCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

private:
    void newConnection(int sockfd, const SockAddr& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;   // The acceptor loop;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MsgCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;

}; // class TcpServer


class TcpConnection : public std::enable_shared_from_this<TcpConnection>{
    
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
public:
    TcpConnection(EventLoop* loop,
                  const std::string& name,
                  int sockfd,
                  const SockAddr& localAddr,
                  const SockAddr& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() {return loop_;}
    const std::string& name() const {return name_;}
    const SockAddr& localAddress() const {return localAddr_;}
    const SockAddr& peerAddress() const {return peerAddr_;}
    template <class T>
    T& getContext() {
        return context_.getContext<T>();
    }
    bool connected() const {return state_ == StateE::kConnected;}

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMsgCallback(const MsgCallback& cb) {
        msgCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

    void connectEstablished();
    void connectDestroyed();
    // Thread safe
    void shutdown();
    // Thread safe
    void send(const std::string& msg);
    // Thread safe
    void close();
    // Thread safe;
    void closeInNextLoop();

    // Not Thread safe, called in loop
    long sendfile(int filefd, long *offset, long count);

private:
    enum class StateE { kConnecting, kConnected, kDisconnecting, kDisconnected, };

    void setState(StateE s) {
        state_ = s;
    }
    void shutdownInLoop();
    void closeInLoop();
    void sendInLoop(const std::string& msg);
    void handleRead(Timestamp recvTime);
    void handleWrite();
    void handleClose();
    void handleError();

    EventLoop* loop_;
    std::string name_;
    StateE state_;
    Buffer input_;
    Buffer output_;
    util::AutoContext context_;
    int sockFd_;
    std::unique_ptr<Channel> connChannel_;
    SockAddr localAddr_;
    SockAddr peerAddr_;
    ConnectionCallback connectionCallback_; // 用户回调
    MsgCallback msgCallback_;               // 用户回调
    CloseCallback closeCallback_;           // 绑定 TcpSever::removeConnection()
    WriteCompleteCallback writeCompleteCallback_; // 用户回调


}; // class TcpConnection

} // namespace miniduo
