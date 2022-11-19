#include "poller.h"
#include "logging.h"
#include "channel.h"

#include <poll.h>
#include <cassert>

using namespace miniduo;

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{

}

Poller::~Poller()
{

}

// Poller::poll() 调用 poll(2) 获得当前活动的IO事件
// 然后填充 调用方传入的 activeChannels, 并返回 poll(2)
// return 的时刻。
time_t Poller::poll(int timeoutMs, ChannelList* activeChannels) {

    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    time_t now = time(0); // get current time;
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


// fillActiveChannels() 遍历 pollfds_, 找出有活动事件的fd，
// 把它对应的 Channel 填入 activeChannels. 复杂度 O(N)
// 当前活动事件 revents 会保存在 Channel中，供Channel::handleEvent()使用
// 注意：不能一边遍历pollfds_, 一边调用 Channel::handleEvent()
void Poller::fillActiveChannels(int numEvents,
                                ChannelList* activeChannels) const
{
    for( PollFdList::const_iterator pfd = pollfds_.begin();
         pfd != pollfds_.end() && numEvents > 0;
         ++pfd )
    {
        // 该 fd 有活动事件
        if(pfd->revents > 0) {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->setRevents(pfd->revents); // 激活 channel 的活动事件
            // pfd->revents = 0;
            activeChannels->push_back(channel); 
        }
    }
}

/// 负责维护和更新pollfds_数组。
/// 添加 Channel 的复杂度 O(logN), 更新已有Channel是 O(1)
void Poller::updateChannelInLooping(Channel* channel) {
    assertInLoopThread();
    // log << "fd = " << channel->fd() << " events = " << channel->events();
    log_trace("Poller update: fd = %d, intrested events = %d", channel->fd(), channel->events());
    if(channel->index() < 0) {
        // a new channel, add to pollfds_
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->setIndex(idx);
        channels_[pfd.fd] = channel;

    }
    else {
        // update existing channel
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        // ??? 当需要删除一些fd时 是否会出现错误
        assert(idx > 0 && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if(channel->isNoneEvent()) {
            // ignore this pollfd, poll(2) 会忽略此pfd
            pfd.fd = -channel->fd()-1;
        }

    }    
}

void Poller::removeChannelInLooping(Channel* channel) {
    ownerLoop_->assertInLoopThread();
    log_trace("fd = %d", channel->fd());
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx];
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);
    if(static_cast<size_t>(idx) == pollfds_.size()-1) {
        pollfds_.pop_back();
    }
    else {
        int endFd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
        if(endFd < 0) {
            endFd = - endFd - 1;
        }
        channels_[endFd]->setIndex(idx);
        pollfds_.pop_back();
    }

}



