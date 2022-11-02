#pragma once
#include <vector>
#include <memory> // shared_ptr weak_ptr
#include <set>
#include <functional> // std::function<>
#include "channel.h" //不添加 timerfdChannel_ 报错

namespace miniduo {

class EventLoop;
class Channel;
class Timer;

typedef int64_t Timestamp; // microseconds since the epoch
// typedef std::pair<Timestamp, Timer*> TimerId;
typedef std::weak_ptr<Timer> TimerId;
typedef std::function<void()> TimerCallback;


// Timestamp getTimeOfNow();

class Timer {
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
public:
    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0)
    {}

    void run() const { callback_(); }
    Timestamp expiration() const { return expiration_;}
    bool repeat() const { return repeat_; }
    // restart the timer, if repeatable, new expiration = now + interval * 10e6
    // if not, expiration = 0;
    void restart(Timestamp now) {
        expiration_ = repeat_ ? now + interval_ * 1000000 : 0;
    };

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;

}; // class Timer

class TimerQueue {
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    
    /// Schedules the callback to be run at given time.
    /// repeats if arg interval > 0.0
    TimerId addTimer(const TimerCallback& cb,
                     Timestamp when,
                     double interval);
    
    // void cancel(TimerId timerId)

private:
    typedef std::pair<Timestamp, std::shared_ptr<Timer>> TimerEntry; /// 
    typedef std::set<TimerEntry> TimerList;

    // canlled when timerfd alarms
    void handleRead();
    // move out all expired timers
    std::vector<TimerEntry> getExpired(Timestamp now);
    /// @brief Add repeatable Timers in arg-expired back to timers_
    void reset(const std::vector<TimerEntry>& expired, Timestamp now);
    /// @brief  Insert a new timer ptr into timers_
    /// @return return true if the earliest expiration in timers_ changed
    bool insert(std::shared_ptr<Timer> timer);
    /// @brief add timer in EventLoop thread
    void addTimerInLoop(std::shared_ptr<Timer> timer);

    EventLoop* loop_; // owner loop;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_; // Timer list sorted by expiration

}; // class TimerQueue

} // namespace miniduo