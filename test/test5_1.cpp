#include "EventLoop.h"
#include <thread>
#include "logging.h"

miniduo::EventLoop* g_loop;

void print() {
    printf("kkkk\n");
}

void threadFunc() {
    g_loop->runAfter(1.0, print);
}

int main() {
    set_logLevel(Logger::LogLevel::DEBUG);
    miniduo::EventLoop loop;
    g_loop = &loop;
    std::thread t(threadFunc);
    loop.loop();
    return 0;
}