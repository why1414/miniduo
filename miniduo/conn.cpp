#include "conn.h"
#include "EventLoop.h"
#include "net.h"
#include "channel.h"
#include "logging.h"

#include <unistd.h>
#include <functional>
#include <cassert>
 #include <sys/sendfile.h> // sendfile()

using namespace miniduo;
// using namespace socket;

Acceptor::Acceptor(EventLoop* loop, const SockAddr& listenAddr) 
    : loop_(loop),
      acceptFd_(socket::createNonblockingSock()),
      acceptChannel_(loop, acceptFd_),
      listening_(false)
{
    socket::setReuseAddr(acceptFd_);
    socket::bindAddr(acceptFd_, listenAddr);
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this, std::placeholders::_1));
}

void Acceptor::listen() {
    // loop_->assertInLoopThread();
    log_trace("enable listen");
    listening_ = true;
    socket::listenSock(acceptFd_);
    loop_->addChannel(&acceptChannel_);
    acceptChannel_.enableReading(true);
}

void Acceptor::handleRead(Timestamp recvTime) {
    loop_->assertInLoopThread();
    SockAddr peerAddr(0);
    int connfd = -1;
    /// FIXME: loop until no more new conn
    // int connfd = socket::acceptSock(acceptFd_, &peerAddr);
    while(acceptFd_ >= 0 && (connfd = socket::acceptSock(acceptFd_, &peerAddr)) >= 0) {
        if(newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        }
        else {
            close(connfd);
        }
    }
    
}

TcpServer::TcpServer(EventLoop* loop, const SockAddr& listenAddr)
    : loop_(loop),
      name_(listenAddr.addrString()),
      acceptor_(new Acceptor(loop, listenAddr)),
      started_(false),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, 
            std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    log_trace("TcpServer::~TcpServer");
    // 释放所有TcpConn
    for(auto& item: connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::start() {
    if(!started_) {
        started_ = true;
    }
    if(!acceptor_->listenning()) {
        loop_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd, const SockAddr& peerAddr) {
    loop_->assertInLoopThread();
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    log_info("TcpServer::newConnection [%s] - new connection [%s] from %s", 
            name_.c_str(), connName.c_str(), peerAddr.addrString().c_str());
    SockAddr localAddr(socket::getLocalAddr(sockfd));
    EventLoop* ioLoop = loop_->allocLoop();
    assert(ioLoop != nullptr);
    TcpConnectionPtr conn( new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMsgCallback(msgCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->getLoop()->runInLoop([conn] {conn->connectEstablished();});
    // conn->connectEstablished();

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    log_trace("step into");
    loop_->runInLoop([this, conn] {removeConnectionInLoop(conn);});
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    log_info("TcpServer::removeConnection [%s] - connection", conn->name().c_str());
    assert( connections_.find(conn->name()) != connections_.end() );
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    // queueInLoop: 确保 TcpConn 不会在 IO 处理中handleclose析构
    conn->getLoop()->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));   
}


TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& name,
                             int sockfd,
                             const SockAddr& localAddr,
                             const SockAddr& peerAddr)
    : loop_(loop),
      name_(name),
      state_(StateE::kConnecting),
      sockFd_(sockfd),
      connChannel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr)
{
    assert(loop_ != nullptr);
    log_debug("TcpConnection::ctor [%s] at %p fd=%d", name_.c_str(), this, sockfd);
    connChannel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    connChannel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );


}

TcpConnection::~TcpConnection() {
    log_trace("TcpConnection::dtor [%s] at %p fd=%d", name_.c_str(), this, sockFd_);
    ::close(sockFd_);
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == StateE::kConnecting);
    setState(StateE::kConnected);
    loop_->addChannel(connChannel_.get());
    connChannel_->enableReading(true);
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    // assert(state_ == StateE::kConnected 
    //         || state_ == StateE::kDisconnecting);
    if(state_ == StateE::kConnected) {
        setState(StateE::kDisconnected);
        connChannel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    loop_->removeChannel(connChannel_.get());
}

void TcpConnection::handleRead(Timestamp recvTime) {
    // char buf[65536];
    // ssize_t n = ::read(connChannel_->fd(), buf, sizeof(buf));
    int savedErrno;
    ssize_t n = input_.readFd(connChannel_->fd(), &savedErrno);
    if(n > 0) {
        msgCallback_(shared_from_this(), &input_, recvTime);
    }
    else if(n==0) {
        handleClose();
    }
    else {
        errno = savedErrno;
        log_error("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    log_trace("TcpConnection::handleClose state = %d", state_);
    assert(state_ == StateE::kConnected 
            || state_ == StateE::kDisconnecting);
    setState(StateE::kDisconnected);
    connChannel_->disableAll();
    connectionCallback_(shared_from_this());
    // loop_->queueInLoop(std::bind(closeCallback_, shared_from_this()));
    // TcpServer::removeConnection
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
    log_error("TcpConnection::handleError()");
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(connChannel_->isWriting()) 
    {
        ssize_t n = 0;
        if(output_.readableBytes() > 0) {
            n = write(connChannel_->fd(),
                          output_.beginRead(),
                          output_.readableBytes());
        }
        if(n >= 0) 
        {
            output_.retrieve(n);
            if(output_.readableBytes() == 0) 
            {
                connChannel_->enableWriting(false);
                if(writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if(state_ == StateE::kDisconnecting) 
                {
                    shutdownInLoop();
                }
            }
            else 
            {
                log_trace("More data to write");
            }
        }
        else 
        {
            log_error("TcpConnection::handleWrite");
        }
    }
    else 
    {
        log_trace("Connection is down, no more writing");
    }
}

void TcpConnection::shutdown() {
    /// FIXME: make it thread safe for state_
    loop_->runInLoop(
        std::bind(&TcpConnection::shutdownInLoop, this));
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if(state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        if(!connChannel_->isWriting()) {
            socket::shutdownWrite(connChannel_->fd());
        }
    }
}

void TcpConnection::close() {
    // 立即关闭当前连接，已在任务队列中的未完成发射任务不会再执行
    loop_->queueInLoop(
        std::bind(&TcpConnection::closeInLoop, this)
    );
}

void TcpConnection::closeInNextLoop() {
    // 延后一个loop循环再关闭tcp连接，
    // 当前循环(loop)中已在任务队列的发送任务会在下一次loop的IO处理中执行
    // 此处使用 两层queueInLoop 而不是 runInLoop，
    // 是为了确保当前loop中，已在任务队列中的发送任务能够先完成
    // closeInLoop() 会在下一次 loop 的任务队列中执行
    log_trace("step into");
    loop_->queueInLoop(
        [this] { loop_->queueInLoop(std::bind(&TcpConnection::closeInLoop, this));}
    );
}

void TcpConnection::closeInLoop() {
    log_trace("step into");
    if(loop_ == nullptr) {
        printf("thread: %d this: %p\n", util::currentTid(), this);

    }
    assert(loop_ != nullptr);
    loop_->assertInLoopThread();
    if(state_ == StateE::kConnected || state_ == StateE::kDisconnecting) {
        // setState(StateE::kDisconnected);
        handleClose();
    }
}

void TcpConnection::send(const std::string& msg) {
    // if msg is empty, this call will only enable writeable event
    assert(msg.size() >= 0); 
    loop_->runInLoop(
        std::bind(&TcpConnection::sendInLoop, this, msg)
    );

}


/// @brief 在loop{中发送msg，将msg放入output buffer中，
/// 并激活监听 writable event
/// @param msg 
void TcpConnection::sendInLoop(const std::string& msg) {
    loop_->assertInLoopThread();
    if(state_ == StateE::kConnected) {
        output_.append(msg.data(), msg.size());
        if(!connChannel_->isWriting()) {
            connChannel_->enableWriting(true);
        }
    }
}

long TcpConnection::sendfile(int filefd, long *offset, long count) {
    assert(filefd > 0);
    loop_->assertInLoopThread();
    return ::sendfile(connChannel_->fd(), filefd, offset, count);
}

