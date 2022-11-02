#include "EventLoop.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "util.h"
#include "logging.h"

int cnt = 0;
miniduo::EventLoop *g_loop;

void printTid(){
    printf("pid = %d, tid = %d\n", getpid(), miniduo::util::currentTid());
    printf("Now %ldus\n", miniduo::util::getTimeOfNow());

}

void print(const char *msg){
    printf("msg %ldus: %s\n", miniduo::util::getTimeOfNow(), msg);
    if(++cnt == 20){
        g_loop->quit();
    }
}

int main()
{
//   set_logLevel(Logger::LogLevel::DEBUG);
  printTid();
  miniduo::EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1.0,   std::bind(print, "once1"));
  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  loop.runEvery(2,   std::bind(print, "every2"));
  loop.runEvery(3,   std::bind(print, "every3"));
  loop.runAfter(4, std::bind(print, "once4"));
  loop.runAfter(5, std::bind(print, "once5"));
  loop.runAfter(6, std::bind(print, "once6"));
  

  loop.loop();
  print("main loop exits");
  sleep(1);
//   return 0;
}
