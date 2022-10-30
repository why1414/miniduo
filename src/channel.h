#pragma once

#include <functional> // function<T>

namespace miniduo{

class EventLoop;

class Channel{
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

public:
    typedef std::function<void()> EventCallback;
    Channel(EventLoop* loop, int fd);

    void handleEvent();
    void setReadCallback(const EventCallback& cb){
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
    // 将 readable event 添加到感兴趣事件中，并在poller更新
    void enableReading() { events_ |= kReadEvent; update(); }
    // void enableWriting() { events_ |= kWriteEvent; update(); }
    // void disableWriting() { events_ &= ~kWriteEvent; update(); }
    // void disableAll() { events_ = kNoneEvent; update(); }

    // for Poller
    int index() { return index_; }
    void setIndex(int idx) { index_ = idx; }

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
    int        index_;   // used by Poller

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_; 

}; // class channel

} // namespace miniduo