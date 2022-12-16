#include "miniduo/EventLoop.h"
#include <thread>
#include <unistd.h> // sleep
#include <iostream>

miniduo::EventLoop* g_loop;

void threadFunc(){
    g_loop->loop();
}

int main(){
    miniduo::EventLoop loop;
    g_loop = &loop;
    std::thread t(threadFunc);
    sleep(30);
    loop.quit();
    std::cout<<"looping in main\n";
    loop.loop();
    t.join();
    return 0;
}