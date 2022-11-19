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
    // thread safe
    void updateChannel(Channel* channel);
    // thread safe
    void removeChannel(Channel* channel);
    // Thread safe (RunInLoop)
    TimerId runAt(const Timestamp time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    // Thread safe (RunInLoop)
    void cancel(TimerId timerId);

    void runInLoop(const Task& cb);
    void queueInLoop(const Task& cb);
    void wakeup();
    

    void assertInLoopThread(){
        if(!isInLoopThread()){
            abortNotInLoopThread();
        }
    }
    /// @brief 判断当前 线程是否有 已经有loop注册，若有检查是不是自己注册的；
    /// 如果没有，就在当前线程注册自己 
    bool isInLoopThread() ;
    pid_t gettid() const {
        return tid_;
    }
    void settid(pid_t tid) {
        tid_ = tid;
    }

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();
    void doPendingTasks();
    
    typedef std::vector<Channel*> ChannelList;
    
    std::atomic<bool> stoplooping_; /* atomic */
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::mutex mutex_;
    std::vector<Task> pendingTasks_; // Guarded by mutex_

    pid_t tid_; // looping thread tid
    
};



}; // namespace miniduo