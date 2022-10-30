#pragma once

// #include <thread>
#include <pthread.h>
#include "timer.h"

namespace miniduo{

struct util {
    static pid_t currentTid();
    static Timestamp getTimeOfNow();
};

} // namespace miniduo