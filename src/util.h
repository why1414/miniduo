#pragma once

// #include <thread>
#include <pthread.h> // pid_t
#include <stdint.h> // int64_t
#include <string> // std::string

namespace miniduo{

typedef int64_t Timestamp; // microseconds since the epoch

namespace util {
    pid_t currentTid();
    Timestamp getTimeOfNow();
    std::string timeString(Timestamp now);
}

} // namespace miniduo