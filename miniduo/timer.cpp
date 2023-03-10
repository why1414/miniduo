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


namespace miniduo {
//  3 timerfd helper funtions
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

void closeTimerfd(int fd) {
    if(fd > 0) {
        ::close(fd);
    }
}

} // namespace miniduo




using namespace miniduo;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_()
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this, std::placeholders::_1)
    );
    // timerfdChannel_.enableReading();
    // loop_->runInLoop(std::bind(&Channel::enableReading, &this->timerfdChannel_));
}

TimerQueue::~TimerQueue() {
    closeTimerfd(timerfd_);
    // delete all Timers
    // ??? do not remove channel, since we're in EventLoop::dtor();
    // Guess: EventLoop is destroying, it is not safe to call removeChannel, 
    // and it's necessary because the poller will be destoryed too.

}

/// thread safe
// TimerId TimerQueue::addTimer(const TimerCallback& cb,
//                              Timestamp when,
//                              double interval)
// {
//     // Timer* timer = new Timer(cb, when, interval);
//     log_trace("Enter addTimer()");
//     auto timer = std::make_shared<Timer>(cb, when, interval);
//     TimerId id = timer;
//     loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, std::move(timer)));
//     return id;
// }     

void TimerQueue::enableChannel() {
    loop_->addChannel(&timerfdChannel_);
    timerfdChannel_.enableReading(true);
}

void TimerQueue::addTimer(std::shared_ptr<Timer> timer) {
    loop_->assertInLoopThread();
    Timestamp when = timer->expiration();
    bool earliestChanged = insert(std::move(timer));
    if(earliestChanged) {
        resetTimerfd(timerfd_, when);
    }
}


void TimerQueue::cancelTimer(TimerId timerId) {
    loop_->assertInLoopThread();
    if(std::shared_ptr<Timer> timer = timerId.lock()) {
        TimerEntry timerEntry(timer->expiration(), timer);
        TimerList::iterator it = timers_.find(timerEntry);
        if(it != timers_.end()) {
            /// FIXME: ??????????????????
            timers_.erase(it);
        }
        else {
            /// FIXME: ?????? queueInLoop ?????????????????????handle timers ?????????cancelTimer()
            /// expired ??? timer ???interval??????0.0 ???????????????????????????????????? reset??? timer ?????????
            timer->setUnrepeat();
        }
    }
}


void TimerQueue::handleRead(Timestamp recvTime) {
    // handleRead will be called in EventLoop::loop()
    loop_->assertInLoopThread();
    log_trace("Handling timers...");
    Timestamp now = util::getTimeOfNow();
    readTimerfd(timerfd_, now);
    std::vector<TimerEntry> expired = getExpired(now);
    log_trace("Total alarmed timers are %d", expired.size());
    for(std::vector<TimerEntry>::iterator it = expired.begin();
        it != expired.end();
        ++it)
    {
        it->second->run();
    }
    // add repeatable Timer back to TimerQueue;
    reset(expired, now);
    log_trace("timers_.size() == %d", timers_.size());
}

// ???timers_??????????????????Timer?????????vector????????????
std::vector<TimerQueue::TimerEntry> TimerQueue::getExpired(Timestamp now) {
    std::vector<TimerEntry> expired;
    
    // ???????????????, now timepoing + 1us; ?????????????????? 1us ????????????????????????????????????
    TimerEntry sentry(now+1, std::shared_ptr<Timer>()); 
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);

    // ??? ?????? copy ??????????????????????????? ??????????????????????????? timers_???
    // ??? timers_ ???set??????????????????????????????????????????????????????????????? it ??????????????????
    // ???????????? set ?????????????????????????????????????????????????????????????????? make_move_iterator,
    // ????????????????????????timers_?????????timer?????????timer ???????????????????????????????????? 1 
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);
    return expired;
}

void TimerQueue::reset(const std::vector<TimerEntry>& expired, Timestamp now) {
    Timestamp nextExpire = 0;
    for(std::vector<TimerEntry>::const_iterator it = expired.begin();
        it != expired.end();
        ++it )
    {
        if(it->second->repeat()) {
            it->second->restart(now);
            insert(std::move(it->second));
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

bool TimerQueue::insert(std::shared_ptr<Timer> timer) {
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    auto result = timers_.insert(std::make_pair(when, std::move(timer)));
    assert(result.second);
    return earliestChanged;
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







