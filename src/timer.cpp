#include "timer.h"
#include "EventLoop.h"
#include "logging.h"
#include "util.h"

#include <unistd.h> // close()
#include <cassert>  // assert()
#include <sys/timerfd.h> // timerfd_*()
#include <cstring> // memset()
#include <sys/time.h> // gettimeofday() unused !!!
#include <chrono> 

using namespace miniduo;

//  timerfd helper funtions
int createTimerfd();
void readTimerfd(int timerfd, Timestamp now);
void resetTimerfd(int timerfd, Timestamp expiration);

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_()
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this)
    );
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    ::close(timerfd_);
    // delete all Timers
    // ??? do not remove channel, since we're in EventLoop::dtor();
    // Guess: in EventLoop::dtor(), there're still some channels, if remove the channel,
    // it will disorder channel vector/array.
    for(TimerList::iterator it = timers_.begin();
        it != timers_.end();
        ++it)
    {
        delete it->second;
    }
}

/// thread safe
TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId{when,timer};
}     

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::handleRead() {
    // handleRead will be called in EventLoop::loop()
    loop_->assertInLoopThread();
    log_trace("Handling timers...");
    Timestamp now = util::getTimeOfNow();
    readTimerfd(timerfd_, now);
    std::vector<TimerId> expired = getExpired(now);
    log_trace("Total alarmed timers are %d", expired.size());
    for(std::vector<TimerId>::iterator it = expired.begin();
        it != expired.end();
        ++it)
    {
        it->second->run();
    }
    // add repeatable Timer back to TimerQueue;
    reset(expired, now);
    log_trace("timers_.size() == %d", timers_.size());
}

// 从timers_中移除已到期Timer，通过vector返回它们
std::vector<TimerId> TimerQueue::getExpired(Timestamp now) {
    std::vector<TimerId> expired;
    // 设置哨兵值
    TimerId sentry{now, reinterpret_cast<Timer*>(UINTPTR_MAX)};
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);
    return expired;
}

void TimerQueue::reset(const std::vector<TimerId>& expired, Timestamp now) {
    Timestamp nextExpire = 0;
    for(std::vector<TimerId>::const_iterator it = expired.begin();
        it != expired.end();
        ++it )
    {
        if(it->second->repeat()) {
            it->second->restart(now);
            insert(it->second);
        }
        else {
            /// FIXME: move to a free list
            delete it->second;
        }
    }
    // reset next alarm timestamp of timerfd_
    if(!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire > 0) {
        resetTimerfd(timerfd_, nextExpire);
    }

}

bool TimerQueue::insert(Timer* timer) {
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    auto result = timers_.insert(std::make_pair(when, timer));
    assert(result.second);
    return earliestChanged;
}

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                    TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0) {
        log_fatal("Failed in timerfd_create");
    }
    return timerfd;
}

void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    log_trace("TimerQueue::handleRead() %ld at %ld ms", howmany, now);
    if(n != sizeof(howmany)) {
        log_fatal("TimerQueue::handleRead() reads %ld bytes instead of 8", n);
    }
}

void resetTimerfd(int timerfd, Timestamp expiration) {
    Timestamp now = util::getTimeOfNow();
    int64_t microseconds = expiration - now;
    // minium interval since next timeout is 100us
    if(microseconds < 100) {
        microseconds = 100;
    }
    
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof newValue);
    newValue.it_value.tv_sec = static_cast<time_t> (microseconds / 1000000);
    newValue.it_value.tv_nsec = static_cast<long> (microseconds % 1000000) * 1000;
    int ret = ::timerfd_settime(timerfd, 0, &newValue, nullptr);
    if(ret) {
        log_trace("timerfd_settime()");
    }

}
// /// @brief 
// /// @return return time (microsecond) since epoch
// Timestamp getTimeOfNow() {
//     // struct timeval tv;
//     // gettimeofday(&tv, nullptr);
//     // int64_t seconds = tv.tv_sec;
//     // return seconds * int(1e6) + tv.tv_usec;
//     auto now = std::chrono::system_clock::now();
//     return (Timestamp)std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
// }







