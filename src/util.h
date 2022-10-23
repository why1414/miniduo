#pragma once

// #include <thread>
#include <pthread.h>

namespace miniduo{

struct util {
    static pid_t currentTid();
};

} // namespace miniduo