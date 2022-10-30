#include "util.h"
#include <unistd.h> // pit_t, ::gettid()
#include <chrono>
#include <sys/time.h>


using namespace miniduo;

__thread pid_t t_cachedTid = 0;

pid_t util::currentTid(){
    if(t_cachedTid == 0){
        t_cachedTid = ::gettid();
    }
    return t_cachedTid;
}

Timestamp util::getTimeOfNow() {
    // struct timeval tv;
    // gettimeofday(&tv, nullptr);
    // int64_t seconds = tv.tv_sec;
    // return seconds * int(1000000) + tv.tv_usec;
    auto now = std::chrono::system_clock::now();
    return (Timestamp)std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}