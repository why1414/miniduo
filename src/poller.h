/*
Poller 是 EventLoop 的间接成员，只供其 owner EventLoop
在 IO 线程调用，因此无须加锁

Poller 是一个抽象基类，通过派生类同时支持 poll 和 epoll
两种 IO multiplexing机制

Poller 生命周期与 EventLoop 相等，Poller 不拥有 Channel
，Channel 在析构前必须自己 unregister (EventLoop::removeChannel())
避免空悬指针

*/

#pragma once
#include <vector>
#include <map>
#include <functional> // std::bind()

#include "EventLoop.h"
#include "util.h" // Timestamp

struct pollfd; // 避免 include<poll.h>

namespace miniduo {

class Channel;

class Poller {

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

public:
    typedef std::vector<Channel*> ChannelList;
    
    Poller(EventLoop* loop);
    ~Poller();

    // Polls the I/O events, called in the loop thread, return current Timepoint.
    Timestamp poll(int timeoutMs, ChannelList* activeChannels);

    /// Thread safe
    void updateChannel(Channel* channel) {
        loop_->runInLoop(
            std::bind(&Poller::updateChannelInLoop, this, channel)
        );
    }
    /// Thread safe
    void removeChannel(Channel* channel) {
        loop_->runInLoop(
            std::bind(&Poller::removeChannelInLoop, this, channel)
        );
    };

    void assertInLoopThread() { loop_->assertInLoopThread(); }



private:
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;
    void updateChannelInLoop(Channel* channel);
    void removeChannelInLoop(Channel* channel);

    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel*> ChannelMap;  // fd -> channel*

    EventLoop* loop_;
    PollFdList pollfds_;   // 缓存 pollfd 数组
    ChannelMap channels_;

};


} // namespace miniduo