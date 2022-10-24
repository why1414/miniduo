#include "EventLoop.h"
#include <thread>

miniduo::EventLoop* g_loop;

void threadFunc(){
    g_loop->loop();
}

int main(){
    miniduo::EventLoop loop;
    g_loop = &loop;
    std::thread t(threadFunc);
    t.join();
    return 0;
}