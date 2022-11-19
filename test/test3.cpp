#include <sys/timerfd.h>
#include <cstring>
#include <unistd.h>
#include "EventLoop.h"
#include "channel.h"
#include "util.h"


miniduo::EventLoop *gLoop;

void timeout(miniduo::Timestamp recvTime)
{
    printf("[ %s ] Timeout! after 5 secs\n", miniduo::util::timeString(recvTime).c_str());
    gLoop->quit();
}

int main()
{

    miniduo::EventLoop loop;
    gLoop = &loop;
    
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                    TFD_NONBLOCK | TFD_CLOEXEC);
    
    miniduo::Channel channel(&loop, timerfd);

    channel.setReadCallback(timeout);
    channel.enableReading();
    

    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, nullptr);
    
    loop.loop();

    ::close(timerfd);
}
