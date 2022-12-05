#pragma once

#include "util.h" // Timestamp


#include <vector>
#include <map>
#include <poll.h> // struct pollfd
#include <sys/epoll.h> // struct epoll_event


namespace miniduo {

class Channel;
class EventLoop;

class BasePoller {
    BasePoller(const BasePoller&) = delete;
    BasePoller& operator=(const BasePoller&) = delete;
public:
    BasePoller(EventLoop* loop): loop_(loop) {}
    virtual ~BasePoller();
    typedef std::vector<Channel*> ChannelList;

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannelList) = 0;
    virtual void addChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    void assertInLoop() const ;
    
private:
    EventLoop* loop_;

}; // class Poller


class PollPoller: public BasePoller {
public:
    
    PollPoller(EventLoop* loop):BasePoller(loop) {}
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void addChannel(Channel* channel) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    typedef std::vector<struct pollfd> PollfdList;
    // fd-> {Channel*, index in pollfds_}
    typedef std::map<int, std::pair<Channel*, int>> ChannelMap;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    PollfdList pollfds_;
    ChannelMap channels_;

}; // class PollPoller


class EPollPoller: public BasePoller {
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void addChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;
    
    void fillActiveChannels(int numEvents, ChannelList* activateChannels) const;
    void update(int operation, Channel* channel);

    enum class ChannelState {
        NEW = 0,
        ADDED,
        DELETED
    };
    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, std::pair<Channel*, ChannelState>> ChannelMap;

    int epollfd_;
    EventList events_;
    ChannelMap channels_;

}; // class EPollPoller


} // namespace miniduo