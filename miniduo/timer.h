#pragma once
#include <vector>
#include <memory> // shared_ptr weak_ptr
#include <set>
#include <functional> // std::function<>
#include "channel.h" //不添加 timerfdChannel_ 报错
#include "util.h" // Timestamp


namespace miniduo {

class Timer;
class EventLoop;
// typedef int64_t Timestamp; // microseconds since the epoch
// typedef std::pair<Timestamp, Timer*> TimerId;
typedef std::weak_ptr<Timer> TimerId;
typedef std::function<void()> TimerCallback;


// Timestamp getTimeOfNow();

class Timer {
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
public:
    Timer(const TimerCallback& cb, const Timestamp& when, const double& interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval)
    {}
    void run() const { callback_(); }
    Timestamp expiration() const { return expiration_;}
    bool repeat() const { return interval_ > 0.0; }
    void setUnrepeat() {
        interval_ = 0.0;
    }
    // restart the timer, if repeatable, new expiration = now + interval * 10e6
    // if not, expiration = 0;
    void restart(Timestamp now) {
        expiration_ = repeat() ? now + interval_ * 1000000 : 0;
    };

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    double interval_;

}; // class Timer

class TimerQueue {
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    /// Schedules the callback to be run at given time.
    /// repeats if arg interval > 0.0
    /// not Thread safe
    void addTimer(std::shared_ptr<Timer> timer);
    /// Not Thread safe
    void cancelTimer(TimerId timerId);
    /// @brief  Called by EventLoop to register the tiemrfd channel to poller
    void enableChannel() ;

private:
    typedef std::pair<Timestamp, std::shared_ptr<Timer>> TimerEntry; /// 
    typedef std::set<TimerEntry> TimerList;

    // canlled when timerfd alarms
    void handleRead(Timestamp recvTime);
    // move out all expired timers
    std::vector<TimerEntry> getExpired(Timestamp now);
    /// @brief Add repeatable Timers in arg-expired back to timers_
    void reset(const std::vector<TimerEntry>& expired, Timestamp now);
    /// @brief  Insert a new timer ptr into timers_
    /// @return return true if the earliest expiration in timers_ changed
    bool insert(std::shared_ptr<Timer> timer);
  
    // void addTimerInLoop(std::shared_ptr<Timer> timer);

    EventLoop* loop_; // owner loop;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_; // Timer list sorted by expiration

}; // class TimerQueue

} // namespace miniduo