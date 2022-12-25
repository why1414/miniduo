## 简介
miniduo 是一个多线程 C++ 网络库，基于 Reactor 模式。Miniduo 主要的设计与实现是基于陈硕的 << Linux多线程服务端编程 >> 中 “muduo网络库设计与实现”。本项目主要是作为学习Linux网络编程与多线程编程的实践项目。

## Features
- 基于非阻塞 I/O 和 I/O 多路复用的 的 Reactor 事件循环，支持 poll 和 epoll
- 基于应用层缓存Buffer模拟实现的 Proactor 模式
- 通过 one loop per thread + 主/从 Reactor 实现了高效的多线程半同步/半异步并发模式。 
    - 半同步：在I/O事件处理完成后，同步处理业务逻辑。
    - 半异步：通过事件循环，异步处理I/O事件，
- 基于该 Reactor 事件循环实现的TCP Server，支持Reactor事件循环上挂载多个TCP Server
- 线程安全的异步日志系统，支持切换文件，支持格式化输出和**流输出**
- 实现定时事件、**信号事件**、与I/O事件的统一处理
- 基于miniduo 实现了一个静态的 Http Server，针对文件传输实现了零拷贝，并解决了大文件传输带来的队头阻塞问题

## 代码示例--echo-server
```c++
#include <miniduo/miniduo.h>
using namespace miniduo;

void onState(const TcpConnectionPtr &conn) {
    if(conn->connected()) {
        log_trace("new connection");
    }
}

void onMsg( const TcpConnectionPtr &conn,
            Buffer *buf,
            Timestamp recvTime) 
{
    conn->send(buf->retrieveAsString());
}

int main(int argc, const char *argv[]) {
    int port = 9981;
    if(argc > 2) {
        printf("./echosvr [port]\n");
        exit(1);
    }
    else if(argc == 2) {
        port = atoi(argv[1]);
    }
    EventLoop loop;
    SockAddr listenAddr(port);
    TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onState);
    server.setMsgCallback(onMsg);
    server.start();
    loop.loop();
    return 0;
}
```

## 安装与使用
测试环境

Ubuntu 20.04.4 LTS

Linux version 5.10.16.3-microsoft-standard-WSL2

gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1) 
```
make && make install
```
## Dirs & Files
- MINDUO/
    - examples/ : 使用miniduo网络库的一些用例
    - drafts/ ：一些源文件的草稿draft
    - resource/ ： Http Server 的根目录
    - miniduo/ ： 项目的头文件与源文件
    - test/ ： 测试源文件
    - workspace/ ： 
    - Makefile : 
    - README.md : 本文件

## TODO：
- [x] rewrite Makefile and 生成静态连接库
- [x] webbench 压力测试
- 时间轮断开空闲连接
- 信号事件统一到IO事件中处理
- [x] 异步日志系统优化


## Http Server 性能测试--webbench
host: 4核8线程
**10000+ clients 10s 请求测试**
### 多线程，1个main-Reactor + 16个sub-Reactor
1. 内存直接响应Response， QPS：10000+
```bash
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /nosuchfile HTTP/1.0
User-Agent: WebBench 1.5
Host: localhost

Runing info: 10500 clients, running 10 sec.

Speed=580638 pages/min, 59949440 bytes/sec.
Requests: 96773 susceed, 0 failed.
```

2. 磁盘文件响应Response，QPS：2000+
```bash
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /index.txt HTTP/1.0
User-Agent: WebBench 1.5
Host: localhost

Runing info: 10500 clients, running 10 sec.

Speed=117072 pages/min, 14446814 bytes/sec.
Requests: 19512 susceed, 0 failed.
```

### 单线程，只有一个 Reactor 事件循环
1. 内存直接响应Response， QPS 1000+
```bash
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /nosuchfile HTTP/1.0
User-Agent: WebBench 1.5
Host: localhost

Runing info: 10500 clients, running 10 sec.

Speed=62148 pages/min, 6444436 bytes/sec.
Requests: 10358 susceed, 0 failed.
```

2. 磁盘文件响应Response，QPS: 300+
```bash
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /index.txt HTTP/1.0
User-Agent: WebBench 1.5
Host: localhost

Runing info: 10500 clients, running 10 sec.

Speed=19260 pages/min, 2022159 bytes/sec.
Requests: 3210 susceed, 0 failed.
```

## References

陈硕, Linux多线程服务端编程, https://book.douban.com/subject/20471211/

游双, Linux高性能服务器编程, https://book.douban.com/subject/24722611/

陈硕, muduo--多线程C++网络库, http://github.com/chenshuo/muduo

yedf2, handy--简洁易用的C++11网络库, https://github.com/yedf2/handy
