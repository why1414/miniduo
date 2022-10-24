#include "EventLoop.h"
#include "util.h"
#include <stdio.h>
#include <thread>
#include <unistd.h> // getpid()

void threadFunc(){
    printf("threadFunc(): pid = %d, tid = %d\n",
    getpid(), miniduo::util::currentTid());

    miniduo::EventLoop loop;
    loop.loop();
}

int main(){
    printf("main(): pid = %d, tid = %d\n",
        getpid(), miniduo::util::currentTid());
    
    miniduo::EventLoop loop;
    std::thread thread(threadFunc);
    loop.loop();
    thread.join();
    return 0;

}