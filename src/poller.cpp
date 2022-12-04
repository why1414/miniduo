#include "poller.h"
#include "channel.h"
#include "EventLoop.h"

#include <poll.h>
#include <cassert>
// #include <pair>

using namespace miniduo;

BasePoller::~BasePoller() {
    log_trace("BasePoller dtor()");
}

void BasePoller::assertInLoop() const {
    loop_->assertInLoopThread();
}

PollPoller::~PollPoller() {
    log_trace("PollPoller is destroyed");
}


Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    Timestamp now = util::getTimeOfNow();
    if(numEvents > 0) {
        // log << numEvents << " events happended";
        log_trace("%d events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    }
    else if(numEvents == 0) {
        // log << " nothing happended";
        log_trace("nothing happened");
    }
    else {
        // log << "Poller::poll()";
        log_error("Poller::poll() return error");
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents,
                                ChannelList* activeChannels) const
{
    for( PollfdList::const_iterator pfd = pollfds_.begin();
         pfd != pollfds_.end() && numEvents > 0;
         ++pfd )
    {
        // 该 fd 有活动事件
        if(pfd->revents > 0) {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second.first;
            assert(channel->fd() == pfd->fd);
            channel->setRevents(pfd->revents); // 激活 channel 的活动事件
            // pfd->revents = 0;
            activeChannels->push_back(channel); 
        }
    }
}

void PollPoller::addChannel(Channel* channel) {
    assertInLoop();
    log_trace("PollPoller add: fd = %d, interested events = %d", channel->fd(), channel->events());
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->isNoneEvent() ? -channel->fd()-1 : channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    channels_[channel->fd()] = {channel, pollfds_.size()};
    pollfds_.push_back(pfd);
}

void PollPoller::updateChannel(Channel* channel) {
    assertInLoop();
    log_trace("PollPoller update: fd = %d, interested events = %d", channel->fd(), channel->events());
    assert(channels_.find(channel->fd()) != channels_.end() );
    assert(channels_[channel->fd()].first == channel);
    int idx = channels_[channel->fd()].second;
    assert(idx >= 0 && idx < pollfds_.size());
    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pfd.fd = channel->fd();
    if(channel->isNoneEvent()) {
        // ignore this pollfd, poll(2) 会忽略此pfd
        pfd.fd = -channel->fd()-1;
    }
}

void PollPoller::removeChannel(Channel* channel) {
    assertInLoop();
    log_trace("PollPoller remove: fd = %d", channel->fd());
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()].first == channel);
    assert(channel->isNoneEvent());
    int idx = channels_[channel->fd()].second;
    assert(0 <= idx && idx < pollfds_.size());
    const struct pollfd& pfd = pollfds_[idx];
    size_t n = channels_.erase(channel->fd());
    assert(n == 1); // 断言成功删除，失败的话 n == 0
    if(static_cast<size_t>(idx) < pollfds_.size()-1) {
        // 与末尾元素交换在pollfds_中的位置
        int endfd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
        if(endfd < 0) { 
            endfd = - endfd - 1;
        }
        // 重置 endfd 的索引
        channels_[endfd].second = idx;
    }
    pollfds_.pop_back();

}