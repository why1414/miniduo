## 项目概述
miniduo 是一个 C++ 多线程服务端的网络库，基于 Reactor 模式。miniduo 主要的设计与实现是基于陈硕的 << Linux多线程服务端编程 >> 中 “muduo网络库设计与实现”。本项目对其中的部分依赖利用 C++ 11 进行了重写，修改与简化了部分实现，如异步日志和多线程Reactor模型。本项目主要是作为学习Linux网络编程与多线程编程的实践项目。

## 项目特点
- 基于非阻塞 I/O 和 I/O 多路复用的 Reactor 事件循环，支持 poll 和 epoll
- 通过主/从 Reactor + one loop per thread 实现了高效的多线程半同步/半异步并发模式。 
    - 半异步：通过 Reactor 事件循环，异步处理I/O事件，
    - 半同步：在I/O事件处理完成后，同步处理业务逻辑。
- 基于该 Reactor 事件循环实现的TCP Server，支持 Reactor 事件循环上挂载多个TCP Server
- 线程安全的异步日志系统，支持切换文件，支持格式化输出和流输出
    - 前台线程写入日志消息队列
    - 后台线程负责将日志写入日志文件
- 实现定时事件、与I/O事件的统一处理
- 基于 miniduo 实现了一个静态的 Http Server，针对大文件传输利用零拷贝函数 sendfile 和 发送完成回调 (WriteCompleteCallback) 进行了优化，避免了发送大文件对于发送缓冲区的过度占用，以及避免了对于同一Reactor中其他连接上请求的阻塞。

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
    int port = 9999;
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

## 安装
测试环境

    Ubuntu 20.04.4 LTS
    Linux  5.10.16.3-microsoft-standard-WSL2 x86_64
    g++    9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1) 

```bash
$ make install
```
## 项目目录

```bash
.
├── examples/    使用 miniduo 网络库的一些用例
├── miniduo/     项目源文件
├── test/        测试源文件
├── webbench/    http server 压力测试工具
├── Makefile     
└── README.md    本文件

```

## TODO：
- [x] 重写 Makefile 和生成静态连接库
- [x] webbench 压力测试
- [x] 异步日志系统优化


## Http Server 性能测试--webbench
host: 4核8线程 intel i5-8250U
**10000+ clients 10s 请求测试**
### 多线程，1个main-Reactor + 16个sub-Reactor
1. 内存直接响应Response， QPS：10000
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

2. 磁盘文件响应Response，QPS：2000
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



## References

陈硕, Linux多线程服务端编程, https://book.douban.com/subject/20471211/

游双, Linux高性能服务器编程, https://book.douban.com/subject/24722611/

陈硕, muduo--多线程C++网络库, http://github.com/chenshuo/muduo

yedf2, handy--简洁易用的C++11网络库, https://github.com/yedf2/handy
