#include "util.h"
#include <unistd.h> // pit_t, ::gettid()
#include <chrono>
#include <sys/time.h>


using namespace miniduo;





namespace miniduo
{
namespace util
{
__thread pid_t t_cachedTid = 0;

pid_t currentTid(){
    if(t_cachedTid == 0){
        t_cachedTid = ::gettid();
    }
    return t_cachedTid;
}

Timestamp getTimeOfNow() {
    // struct timeval tv;
    // gettimeofday(&tv, nullptr);
    // int64_t seconds = tv.tv_sec;
    // return seconds * int(1000000) + tv.tv_usec;
    auto now = std::chrono::system_clock::now();
    return (Timestamp)std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

std::string timeString(Timestamp now) {
    time_t tt = now / 1e6;
    struct tm timeinfo;
    localtime_r(&tt, &timeinfo);
    char timeStr[100];
    snprintf(timeStr, sizeof(timeStr),"%04d-%02d-%02d %02d:%02d:%02d.%3ld",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             (now/1000)%1000);
    
    return std::string(timeStr);
}

} // namespace util
} // namespace miniduo
