#pragma once

#include "buffer.h" // Class Buffer
#include "util.h"   // Timestamp
 
#include <memory>
#include <functional>

namespace miniduo
{
class TcpConnection;
typedef std::function<void()> TimerCallback;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
// typedef std::function<void (const TcpConnectionPtr&,
//                             const char* data,
//                             ssize_t len)> MsgCallback;
typedef std::function<void (const TcpConnectionPtr&,
                            Buffer* buf,
                            Timestamp)> MsgCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;

} // namespace miniduo
