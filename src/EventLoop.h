#pragma once

// #include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <memory>

#include "util.h"

namespace miniduo{

class Channel;
class Poller;

class EventLoop {
    EventLoop(const EventLoop &) = delete;
    EventLoop& operator= (const EventLoop&) = delete;

public:
    EventLoop();
    ~EventLoop();

    void loop();

    void quit();
    void updateChannel(Channel* channel);
    
    void assertInLoopThread(){
        if(!isInLoopThread()){
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const {
        return threadId_ == util::currentTid();
    }

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();

    typedef std::vector<Channel*> ChannelList;
    
    bool looping_; /* atomic */
    bool quit_ ;  /* atomic */
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    ChannelList activeChannels_;
};



}; // namespace miniduo