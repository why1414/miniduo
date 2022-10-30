#pragma once

// #include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <memory>

#include "timer.h"
#include "util.h"

namespace miniduo{

class Channel;
class Poller;
class TimerQueue;

class EventLoop {
    EventLoop(const EventLoop &) = delete;
    EventLoop& operator= (const EventLoop&) = delete;

public:
    EventLoop();
    ~EventLoop();

    void loop();

    void quit();
    void updateChannel(Channel* channel);
    TimerId runAt(const Timestamp time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    
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
    bool quit_ ;   /* atomic */
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;

    
};



}; // namespace miniduo