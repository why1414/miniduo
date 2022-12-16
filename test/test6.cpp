#include "miniduo/EventLoop.h"
#include "miniduo/util.h"
#include "miniduo/logging.h"
#include <unistd.h>
#include <thread>
#include <mutex>

using namespace miniduo;

EventLoop* g_loop = nullptr;
std::mutex lk;

void threadFunc() {
    printf("threadFunc(): pid = %d, tid = %d\n",
            getpid(), miniduo::util::currentTid());
}

int main() {
    printf("main(): pid = %d, tid = %d\n",
            getpid(), miniduo::util::currentTid());
    // miniduo::EventLoop loop;
    std::thread t([] { static EventLoop tloop;
                       g_loop = &tloop;
                      
                       tloop.loop();});
    sleep(5);
    g_loop->runInLoop(threadFunc);
    g_loop->runEvery(3, threadFunc);
    
    sleep(15);
    g_loop->quit();

    t.join();
    set_logLevel(Logger::LogLevel::DEBUG);
    printf("restarting the loop\n");
    g_loop->loop();
    printf("exit main().\n");
    
    return 0;
}
