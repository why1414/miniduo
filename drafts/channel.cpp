#include "channel.h"
#include "EventLoop.h"
#include "logging.h"

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
void Channel::handleEvent(Timestamp recvTime){
    if(revents_ & POLLNVAL) {
        // log << "Channel::handleEvent() POLLNVAL";
        log_trace("Channel::handleEvent() POLLNVAL");
    }

    if((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        log_warn("Channel::handleEvent() POLLHUP");
        if(closeCallback_) {
            closeCallback_();
        }
    }

    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_(recvTime);
        }
    }

    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}
