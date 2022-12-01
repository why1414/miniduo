#include "logging.h"


#include <iostream>
#include <ctime>
#include <unistd.h>
#include <stdarg.h> // va_*
#include <sstream>
#include <chrono>




Logger::Logger(LogLevel level = LogLevel::TRACE,
               std::string fileBaseName = "log",
               long rotateInterval = 24 * 60 * 60)
    : level_(level),
      fileBaseName_(fileBaseName),
      rotateInterval_(rotateInterval),
      lastRotate_(time(nullptr)),
      fd_(1),
      stopLogging_(false),
      backendThread_(std::thread(&Logger::process, this))

{

}

Logger::~Logger() {
    {
        std::unique_lock<std::mutex> lock(mut_);
        stopLogging_ = true;
        msgQueue_.push("[Logger destroyed] \n");
        cv_.notify_one();        
    }
    backendThread_.join();
    
    
}

void Logger::process() {
    while(!stopLogging_) {
        // 检查fd_
        tryRotate();

        std::unique_lock<std::mutex> lock(mut_);

        while(msgQueue_.empty() && !stopLogging_) {
            cv_.wait(lock);
        }

        while(!msgQueue_.empty()) {
            std::string msg = msgQueue_.front();
            msgQueue_.pop();
            lock.unlock();
            // 写入文件
            int err = write(fd_, msg.c_str(), msg.size());
            lock.lock();
        }
             

    }

}


void Logger::logv(const LogLevel level, 
                const char* fmt, ...) 
{

    if (level < level_) return ;

    static __thread pid_t currTid = ::gettid();
    char buffer[ 4096 ];
    std::ostringstream msgStream;

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    msgStream << "[" << strTime() << "]"
              << "[" << currTid   << "]" 
              << buffer << std::endl;
    
    std::unique_lock<std::mutex> lock(mut_);
    msgQueue_.push(msgStream.str());
    lock.unlock();
    cv_.notify_one();
}

/// TODO: 
void Logger::tryRotate() {

}

Logger& Logger::getLogger() {
    static Logger log;
    return log;
}

// 
std::string Logger::strTime() {
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    // struct tm* timeinfo = localtime(&tt); // not thread safe
    localtime_r(&tt, &timeinfo); // thread safe
    
    char timeStr [100];
    snprintf(timeStr, sizeof(timeStr),"%04d-%02d-%02d %02d:%02d:%02d.%3d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             (int)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000));
    
    return std::string(timeStr);
}


void Logger::setLogLevel(LogLevel level) {
    level_ = level;
}

void Logger::setFileBaseName(std::string basename) {
    fileBaseName_ = basename;
}

void Logger::setRotateInterval(long interval) {
    rotateInterval_ = interval;
}