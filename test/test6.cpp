#include "EventLoop.h"
#include "util.h"
#include <unistd.h>
#include <thread>

using namespace miniduo;

EventLoop* g_loop = nullptr;

void threadFunc() {
    printf("threadFunc(): pid = %d, tid = %d\n",
            getpid(), miniduo::util::currentTid());
    printf("thread: kkkk\n");
}

int main() {
    printf("main(): pid = %d, tid = %d\n",
            getpid(), miniduo::util::currentTid());
    // miniduo::EventLoop loop;
    std::thread t([] { EventLoop tloop;
                       g_loop = &tloop;
                       tloop.loop();});
    sleep(3);
    g_loop->runInLoop(threadFunc);
    g_loop->runAfter(3, threadFunc);
    
    sleep(30);
    g_loop->quit();
    printf("exit main().\n");
    t.join();
    return 0;
}
