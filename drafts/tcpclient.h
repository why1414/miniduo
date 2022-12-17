#pragma once

#include "conn.h"
#include "EventLoop.h"

namespace miniduo {

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class Connector {
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
public:

    typedef std::function<void (int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop, const SockAddr& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback cb) {
        newConnectionCallback_ = cb;
    }

    void start();
    void restart();
    void stop(); 

    const SockAddr& serverAddr() const {
        return serverAddr_;
    }

private:
    enum class State { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(State s) { state_ = s; }
    void startInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* loop_;
    SockAddr serverAddr_;
    std::atomic<bool> connect_;
    State state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
    TimerId timerId_;

};

}