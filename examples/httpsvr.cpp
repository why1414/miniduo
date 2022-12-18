#include <miniduo/miniduo.h>
#include <string>

using namespace miniduo;

int main(int argc, const char* argv[]) {
    int port = 9982;
    std::string resPath = "/mnt/e/GitHub/miniduo_/resource";
    if(argc > 2) {
        printf("./httpsvr [port]\n");
        exit(1);
    }
    else if(argc == 2) {
        port = atoi(argv[1]);
    }
    // set_logLevel(Logger::LogLevel::DISABLE);
    set_logName("httpsvr-log");
    set_logInterval(60); // 60s
    EventLoops loop;
    SockAddr listenAddr(port);
    HttpServer webserver(&loop, listenAddr);
    webserver.setResourcePath(resPath);
    webserver.start();
    loop.loop();
    return 0;
}