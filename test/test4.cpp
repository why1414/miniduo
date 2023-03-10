
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "miniduo/EventLoop.h"
#include "miniduo/util.h"
#include "miniduo/logging.h"
#include "miniduo/timer.h"

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
  set_logLevel(Logger::LogLevel::DEBUG);
  printTid();
  miniduo::EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1,   std::bind(print, "once1"));
  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  miniduo::TimerId id = loop.runEvery(2,   std::bind(print, "every2"));
  miniduo::TimerId id3 = loop.runEvery(3,   std::bind(print, "every3"));
  loop.runAfter(4, std::bind(print, "once4"));
  loop.runAfter(5, std::bind(print, "once5"));
  loop.runAfter(6.1, std::bind(print, "once6"));

  loop.cancel(id);
  loop.runAfter(18.001, std::bind(&miniduo::EventLoop::cancel, &loop, id3));

  loop.loop();
  print("main loop exits");
  sleep(1);
//   return 0;
}
