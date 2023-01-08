#include "miniduo/miniduo.h"
#include <string>

using namespace miniduo;

int main(int argc, const char* argv[]) {
    int port = 9982;
    std::string resPath = "/mnt/e/GitHub/miniduo_/examples/resource";
    if(argc > 2) {
        printf("./httpsvr [port]\n");
        exit(1);
    }
    else if(argc == 2) {
        port = atoi(argv[1]);
    }
    set_logLevel(Logger::LogLevel::TRACE);
    set_logName(argv[0]);
    // set_logInterval(3*60); // second
    set_logSwitchToFileLog();
    EventLoops loop(16);
    SockAddr listenAddr(port);
    HttpServer webserver(&loop, listenAddr);
    webserver.setResourcePath(resPath);
    webserver.start();
    loop.loop();
    return 0;
}