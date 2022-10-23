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

#include "EventLoop.h"

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

    // Polls the I/O events, called in the loop thread.
    time_t poll(int timeoutMs, ChannelList* activeChannels);

    // Changes the interested I/O events, called in the loop thread.
    void updateChannel(Channel* channel);

    void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }



private:
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;
    
    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel*> ChannelMap;  // fd -> channel*

    EventLoop* ownerLoop_;
    PollFdList pollfds_;   // 缓存 pollfd 数组
    ChannelMap channels_;

};


} // namespace miniduo