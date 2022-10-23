#include "../src/EventLoop.h"
#include "../src/util.h"
#include <stdio.h>
#include <thread>

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