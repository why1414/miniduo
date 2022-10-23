#include "channel.h"
#include "EventLoop.h"
#include "log.h"

#include <poll.h>

using namespace miniduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(0),
      revents_(0),
      index_(-1)
{

}

// Channel::update() 会调用 EventLoop::updateChannel() ，
// 后者调用 Poller::updateChannel() .
void Channel::update(){
    loop_->updateChannel(this);
}

// Channel::handleEvent() 根据revents_的值分别调用不同的用户回调
void Channel::handleEvent(){
    if(revents_ & POLLNVAL) {
        log << "Channel::handleEvent() POLLNVAL";
    }

    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_();
        }
    }

    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}
