#pragma once

#include "logging.h"

// #include <thread>
#include <pthread.h> // pid_t
#include <stdint.h> // int64_t
#include <string> // std::string
#include <functional> // std::function

namespace miniduo{

typedef int64_t Timestamp; // microseconds since the epoch

namespace util {

pid_t currentTid();
Timestamp getTimeOfNow();
std::string timeString(Timestamp now);

class AutoContext {
    AutoContext(const AutoContext &) = delete;
    AutoContext& operator=(const AutoContext &) = delete;
public:
    AutoContext(): context_(nullptr) {}
    ~AutoContext() {
        log_trace("AutoContext dtor!!!!");
        if(context_ != nullptr) {
            contextDelete_();
        }
    }
    template <class T>
    T &getContext() {
        if(context_ == nullptr) {
            context_ = new T();
            contextDelete_ = [this] { delete (T*) context_; };
        }
        return *(static_cast<T*> (context_));
    }

private:
    void *context_;
    std::function<void()> contextDelete_;
}; // class AutoContext

} // namespace util

} // namespace miniduo