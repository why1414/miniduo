#include "tcpclient.h"

#include "logging.h"

#include <cassert>

using namespace miniduo;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const SockAddr& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(State::kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    log_debug("ctor[%p]", this);
}

Connector::~Connector()
{
    log_debug("dtor[%p]", this);
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == State::kDisconnected);
    if(connect_) {
        connect();
    }
    else {
        log_debug("do not connect");
    }
}

void Connector::connect() {
    int sockfd = socket::createNonblockingSock();
    int ret = socket::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;
    
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        log_error("connect error in Connector::startInLoop %d ", savedErrno);
        socket::close(sockfd);
        break;
    
    default:
        log_error("Unexpected error in Connector::startInLoop %d", savedErrno);
        socket::close(sockfd);
        break;
    }
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(State::kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop() {
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd) {
    setState(State::kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
        std::bind(&Connector::handleWrite, this)
    );
    channel_->setErrorCallback(
        std::bind(&Connector::handleError, this)
    );
    channel_->enableWriting(true);
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    loop_->removeChannel(channel_.get());
    int sockfd = channel_->fd();
    loop_->queueInLoop(
        std::bind(&Connector::resetChannel, this)
    );
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    log_trace("Connector::handleWrite %d", state_);
    if(state_ == State::kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        if(err) {
            log_warn("Connector::handleWrite - SO_ERROR = %d", err);
            retry(sockfd);
        }
        else if(socket::isSelfConnect(sockfd)) {
            log_warn("Connector::handleWrite - Self connect");
            retry(sockfd);
        }
        else {
            setState(State::kConnected);
            if(connect_) {
                if(newConnectionCallback_)
                    {newConnectionCallback_(sockfd);}
            }
            else {
                socket::close(sockfd);
            }
        }
    }
    else {
        assert(state_ == State::kDisconnected);
    }
}

void Connector::handleError() {
    log_error("Connector::handleError");
    assert(state_ == State::kConnecting);
    int sockfd = removeAndResetChannel();
    int err = socket::getSocketError(sockfd);
    log_trace("SO_ERROR = %d", err);
    retry(sockfd);
}

void Connector::retry(int sockfd) {
    socket::close(sockfd);
    setState(State::kDisconnected);
    if(connect_) {
        log_info("Connector::retry - Retry Connecting to %s in %ld Ms",
                 serverAddr_.addrString().c_str(),
                 retryDelayMs_);
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0,
            std::bind(&Connector::startInLoop, this));  
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs); 
    }
    else {
        log_debug("do not connect");
    }
}

