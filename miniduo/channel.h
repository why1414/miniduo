#pragma once

#include <functional> // function<T>
#include "util.h" // Timestamp
// #include "EventLoop.h"

namespace miniduo{

class EventLoop;

class Channel{
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;
    Channel(EventLoop* loop, int fd);

    void handleEvent(Timestamp recvTime);
    void setReadCallback(const ReadEventCallback& cb){
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback& cb){
        writeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback& cb){
        errorCallback_ =cb;
    }
    

    int fd() const { return fd_; }
    int events() const { return events_; }          
    void setRevents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    // 将 readable event 添加到感兴趣事件中，并在poller更新
    void enableReading(bool enable);
    void enableWriting(bool enable);
    void disableAll() { events_ = kNoneEvent; update(); }

    EventLoop* ownerLoop() { return loop_; }

private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int  fd_;
    int        events_;  // channel 关心的 IO 事件
    int        revents_; // 当前活动的事件，由 EventLoop/Poller 设置

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_; 
    EventCallback closeCallback_;

}; // class channel

} // namespace miniduo