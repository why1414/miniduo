#pragma once

// #include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <memory> // std::unique_ptr<T>
#include <atomic> // std::atomic<T>
#include <mutex>

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
    typedef std::function<void()> Task;

    void loop();
    void quit();
    void updateChannel(Channel* channel);
    TimerId runAt(const Timestamp time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    void cancel(TimerId timerId);

    void runInLoop(const Task& cb);
    void queueInLoop(const Task& cb);
    void wakeup();
    

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
    void handleRead();
    void doPendingTasks();
    
    typedef std::vector<Channel*> ChannelList;
    
    std::atomic<bool> looping_; /* atomic */
    std::atomic<bool> quit_ ;   /* atomic */
    std::atomic<bool> callingPendingTasking_;
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::mutex mutex_;
    std::vector<Task> pendingTasks_; // Guarded by mutex_

    
};



}; // namespace miniduo