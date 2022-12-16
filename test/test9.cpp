#include "miniduo/conn.h"
#include "miniduo/EventLoop.h"
#include "miniduo/net.h"
#include "miniduo/callbacks.h"

#include <unistd.h>

using namespace miniduo;

void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(),
           conn->peerAddress().addrString().c_str());
  }
  else
  {
    printf("onConnection(): connection [%s] is down\n",
           conn->name().c_str());
  }
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(),
         conn->name().c_str(),
         util::timeString(receiveTime).c_str());

  conn->send(buf->retrieveAsString());
}

int main()
{
  printf("main(): pid = %d\n", getpid());

  SockAddr listenAddr(9981);
  EventLoop loop;

  TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMsgCallback(onMessage);
  server.start();

  loop.loop();
}