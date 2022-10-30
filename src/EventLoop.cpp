#include "EventLoop.h"
#include "log.h"
#include "logging.h"
#include "channel.h"
#include "poller.h"


#include <cassert>
#include <poll.h> 
#include <iostream>

using namespace miniduo;
__thread EventLoop* t_loopInThisThread = 0;

extern Timestamp getTimeOfNow();
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      poller_(new Poller(this)),
      threadId_(util::currentTid()),
      timerQueue_(new TimerQueue(this))
{
    // 检查当前 thread 是否已存在 EventLoop
    // log trace EventLoop created

    log_trace("EventLoop created %p in thread %d", this, threadId_);
    if (t_loopInThisThread){
        // log fatal another EventLoop exists in this thread
        log_fatal("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    }
    else {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop(){
    assert(!looping_);
    t_loopInThisThread = nullptr;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread(){
    return t_loopInThisThread;
}

void EventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while(!quit_) {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);
        for(ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end();
            ++it)
        {
            (*it)->handleEvent();
        }

    }

    // ::poll(nullptr, 0, 5*1000);

    // log trace Eventloop stop looping
    log_trace("EventLoop %p stop looping", this);
    looping_ = false;
}

void EventLoop::abortNotInLoopThread(){
    // log
    // log<<" Abort\n";
    log_fatal("Abort! not in Loop Thread");
    pthread_exit(nullptr);
}

// 如果 loop() 正阻塞在某个调用，quit() 不会立刻生效
void EventLoop::quit() {
    quit_ = true;
    // wakeup()
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp time, const TimerCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (int64_t) (delay * 1000000);
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (int64_t) (interval * 1000000);
    return timerQueue_->addTimer(cb, time, interval * 1000000);
}



