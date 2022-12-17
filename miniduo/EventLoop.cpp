#include "EventLoop.h"
#include "logging.h"
#include "channel.h"
#include "poller.h"


#include <cassert>
#include <poll.h> 
#include <iostream>
#include <unistd.h>
#include <sys/eventfd.h> // ::eventfd()

namespace  miniduo
{

class IgnoreSigPipe {
public:
    IgnoreSigPipe() {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
// 全局变量，初始化以忽略 SIGPIPE 信号，防止网络服务器socket异常读写退出
IgnoreSigPipe ignPipe; 


// extern Timestamp getTimeOfNow();
const int kPollTimeMs = 10000;

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0) {
        log_fatal("Failed in eventfd");
        abort(); // stdlib.h
    }
    return evtfd;
}

void closeEventfd(int fd) {
    if(fd < 0) return ;
    ::close(fd);
}

} // namespace  miniduo



using namespace miniduo;



/// FIXME: 同一线程创建多个EventLoop 会冲突， 在timerQueue，和wakeupChannel往 poller中注册时
EventLoop::EventLoop()
    : stoplooping_(true),
      tid_(-1),
      poller_(new EPollPoller(this)),     
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      timerQueue_(new TimerQueue(this))
{   
    // 检查当前 thread 是否已存在 EventLoop
    // log trace EventLoop created
    log_trace("EventLoop created %p in thread %d", this, util::currentTid());
    
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this, std::placeholders::_1)
    );
    addChannel(wakeupChannel_.get());
    wakeupChannel_->enableReading(true);
    // runInLoop(std::bind(&Channel::enableReading, this->wakeupChannel_.get()));
    timerQueue_->enableChannel();
 
}

EventLoop::~EventLoop(){
    assert(stoplooping_);
    closeEventfd(wakeupFd_);
}



bool EventLoop::isInLoopThread() {
    if(!stoplooping_) {
        return tid_ == util::currentTid();
    }
    else {
        return false;
    }
}

void EventLoop::loop(){
    assert(stoplooping_);
    settid(util::currentTid());
    stoplooping_ = false;
    assertInLoopThread();
    log_trace("EventLoop %p Looping starts!", this);
    while(!stoplooping_) {
        doPendingTasks();
        activeChannels_.clear();
        Timestamp recvTime = poller_->poll(kPollTimeMs, &activeChannels_);
        for(ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end();
            ++it)
        {
            (*it)->handleEvent(recvTime);
        }
        
    }
    settid(-1);


    // log trace Eventloop stop looping
    log_trace("EventLoop %p stop looping", this);
}

void EventLoop::abortNotInLoopThread(){
    log_fatal("Abort! not in Loop Thread");
    // pthread_exit(nullptr);
    abort();
}

// 如果 loop() 正阻塞在polling，quit() 不会立刻生效
// 需要其他线程唤醒IO线程，则polling马上返回 
void EventLoop::quit() {
    
    if(!isInLoopThread()) {
        stoplooping_ = true;
        wakeup();
    }
    else { // 在 looping 中，意味着没有阻塞在 polling上
        stoplooping_ = true;
    }
}

void EventLoop::addChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    runInLoop(std::bind(&BasePoller::addChannel, poller_.get(), channel));
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    runInLoop(std::bind(&BasePoller::updateChannel, poller_.get(), channel));
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    runInLoop(std::bind(&BasePoller::removeChannel, poller_.get(), channel));
}

TimerId EventLoop::runAt(const Timestamp time, const TimerCallback &cb) {
    auto timer = std::make_shared<Timer> (cb, time, 0.0);
    TimerId id = timer;
    runInLoop(std::bind(&TimerQueue::addTimer, timerQueue_.get(), std::move(timer)));
    return id;
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (Timestamp) (delay * 1000000);
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (Timestamp) (interval * 1000000);
    auto timer = std::make_shared<Timer> (cb, time, interval);
    TimerId id = timer;
    runInLoop(std::bind(&TimerQueue::addTimer, timerQueue_.get(), std::move(timer)));
    return id;
}

void EventLoop::cancel(TimerId timerId) {
    runInLoop(std::bind(&TimerQueue::cancelTimer, timerQueue_.get(), timerId));
}

void EventLoop::runInLoop(const Task& cb) {
    if(isInLoopThread()) {
        cb();
    }
    else {
        queueInLoop(cb);
    }
    
}

void EventLoop::queueInLoop(const Task& cb) {
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingTasks_.push_back(cb);
    }
    wakeup();
}

void EventLoop::doPendingTasks() {
    std::vector<Task> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(pendingTasks_);
    }
    for(const Task& task: tasks) {
        task();
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        log_error("EventLoop::wakeup() writes %ld bytes instread %ld",n ,sizeof one);
    }
    
} 

void EventLoop::handleRead(Timestamp recvTime) {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        log_error("EventLoop::handleRead() reads %ld bytes instead of %ld", n, sizeof one);
    }

}

EventLoop* EventLoop::allocLoop() {
    return this;
}

EventLoops::EventLoops(int loopNum)
    : size_(loopNum),
      next_(0),
      subLoops_(loopNum)   
{

}

EventLoops::~EventLoops() {

}

EventLoop* EventLoops::allocLoop() {
    if(size_ == 0) {
        return this;
    }
    EventLoop* loop = &subLoops_[next_];
    next_ = (next_ + 1) % size_;
    return loop;
}

/// @brief  创建线程依次启动loops 以及loops全部结束回收线程
void EventLoops::loop() {
    std::thread threads[size_];
    for(int i=0; i<size_; i++) {
        std::thread t(
            std::bind(&EventLoop::loop, &subLoops_[i])
        );
        threads[i].swap(t);
    }
    EventLoop::loop();
    for(auto &t: threads) {
        t.join();
    }
}

void EventLoops::quit() {
    for(auto &loop: subLoops_){
        loop.quit();
    }
    EventLoop::quit();    
}



