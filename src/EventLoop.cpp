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

__thread EventLoop* t_loopInThisThread = nullptr;

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

} // namespace  miniduo



using namespace miniduo;



/// FIXME: 同一线程创建多个EventLoop 会冲突， 在timerQueue，和wakeupChannel往 poller中注册时
EventLoop::EventLoop()
    : stoplooping_(true),
      tid_(-1),
      poller_(new Poller(this)),     
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      timerQueue_(new TimerQueue(this))
{   
    // 检查当前 thread 是否已存在 EventLoop
    // log trace EventLoop created
    log_trace("EventLoop created %p in thread %d", this, util::currentTid());
    if (t_loopInThisThread != nullptr){
        // log fatal another EventLoop exists in this thread
        log_trace("Another EventLoop %p exists in this thread %d", t_loopInThisThread, util::currentTid());
    }
    
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead,this, std::placeholders::_1)
    );
    wakeupChannel_->enableReading();
    // runInLoop(std::bind(&Channel::enableReading, this->wakeupChannel_.get()));
    timerQueue_->enableChannel();
 
}

EventLoop::~EventLoop(){
    assert(stoplooping_);
    if(isInLoopThread()) {
        t_loopInThisThread = nullptr;
    }
    
}

EventLoop* EventLoop::getEventLoopOfCurrentThread(){
    return t_loopInThisThread;
}

bool EventLoop::isInLoopThread() {
    // if(t_loopInThisThread == nullptr) {
    //     t_loopInThisThread = this;
    // }
    // return t_loopInThisThread == this;

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

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp time, const TimerCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (Timestamp) (delay * 1000000);
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time = util::getTimeOfNow() + (Timestamp) (interval * 1000000);
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId) {
    timerQueue_->cancelTimer(timerId);
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



