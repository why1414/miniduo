#include "logging.h"


#include <iostream>
#include <ctime>
#include <unistd.h>
#include <stdarg.h> // va_*
#include <sstream>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // ::open()
#include <cstring> // ::strerror()
#include <cassert>


#define FLUSH_SIZE 4 * 1024


Logger::Logger()
    : level_(LogLevel::TRACE),
      fileDir_("log/"),
      fileBaseName_("log"),
      rotateInterval_(24 * 60 * 60),
      lastRotate_(::time(nullptr)),
      fd_(1),
      stopLogging_(false),
      isfilelog_(false),
      backendThread_(std::thread(&Logger::process, this))

{   
    ::mkdir(fileDir_.c_str(), 0);
    // msgQueue_.push_back("[Logger created] \n");
    msg_.append("[Logger created] \n");
}

Logger::~Logger() {
    {
        std::unique_lock<std::mutex> lock(mut_);
        stopLogging_ = true;
        // msgQueue_.push_back("[Logger destroyed] \n");
        msg_.append("[Logger destroyed] \n");

        cv_.notify_one();        
    }
    backendThread_.join();
    if(fd_ != 1) ::close(fd_);
    
    
}

void Logger::process() {
    while(!stopLogging_) {
        // 检查fd_
        if(isfilelog_) {
            tryRotate();
        }

        // std::list<std::string> msgOutputQueue_;

        // std::unique_lock<std::mutex> lock(mut_);
        // while(msgQueue_.empty() && !stopLogging_) {
        //     cv_.wait(lock);
        // }
        // msgOutputQueue_.swap(msgQueue_);
        // lock.unlock();

        // char buf[4 * 1024];

        // while(!msgOutputQueue_.empty()) {
        //     int len = 0;
        //     while (!msgOutputQueue_.empty() && (sizeof(buf) - len) > msgOutputQueue_.front().size()) {
        //         std::string msg(std::move(msgOutputQueue_.front()));
        //         msgOutputQueue_.pop_front();
        //         std::copy(msg.begin(), msg.end(), buf+len);
        //         len += msg.size();
        //     }
        //     // std::string msg = msgOutputQueue_.front();
        //     // msgOutputQueue_.pop_front();
        //     // 写入文件
        //     // int err = ::write(fd_, msg.c_str(), msg.length());
        //     int err = ::write(fd_, buf, len);
        // }


        std::string msg;
        std::unique_lock<std::mutex> lock(mut_);
        while(msg_.empty() && !stopLogging_) {
            cv_.wait(lock);
        }
        msg.swap(msg_);
        lock.unlock();
        const char *buf = msg.data();
        int tosend = msg.size();
        while(tosend > 0) {
            int ret = ::write(fd_, buf, tosend);
            if(ret > 0) {
                buf += ret;
                tosend -= ret;
            }
            // printf("tosend %d\n", tosend);
        }
        // printf("kkkk\n");

             

    }

}

void Logger::switchToFileLog() {
    assert(fd_ == 1);
    int newfd = openNewLogfile();
    setfd(newfd);
    // isfilelog_ = true;
    setfilelog(true);
}

int Logger::openNewLogfile() {
    assert(lastRotate_ > 0);
    struct tm ntm;
    ::localtime_r(&lastRotate_, &ntm);
    char newname[4096];
    ::snprintf( newname, sizeof(newname), 
                "%s%s.%d%02d%02d%02d%02d", 
                fileDir_.c_str(), fileBaseName_.c_str(), 
                ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday, ntm.tm_hour, ntm.tm_min);
    int fd = ::open(newname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
    if(fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", newname, ::strerror(errno));
        exit(1);
    }
    return fd;
}

Logger::LogStream Logger::streamLogv(LogLevel level) {
    return LogStream(*this, level);
}


void Logger::logv(const LogLevel level, 
                const char* fmt, ...) 
{

    if (level < level_) return ;

    char buffer[ 4096 ];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    flush(std::string(buffer, n));
    
}


void Logger::tryRotate() {
    if(!isfilelog_) return ;
    time_t now = ::time(nullptr);
    if((now - lastRotate_) < rotateInterval_ ) {
        return ;
    }
    lastRotate_ = now;
    ::close(fd_);
    int newfd = openNewLogfile();
    setfd(newfd);

}

Logger& Logger::getLogger() {
    static Logger log;
    return log;
}


std::string Logger::timeNow() {
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    // struct tm* timeinfo = localtime(&tt); // not thread safe
    localtime_r(&tt, &timeinfo); // thread safe
    
    char timeStr [100];
    int n = snprintf(timeStr, sizeof(timeStr),"%04d-%02d-%02d %02d:%02d:%02d.%3d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             (int)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000));
    assert(n >= 0);
    return std::string(timeStr, n);
}

std::string Logger::tid() {
    static __thread pid_t currTid = ::gettid();
    return std::to_string(currTid);
}

void Logger::flush(const std::string &msg) {
    std::string &&logmsg = "[" + timeNow() + "]" +
                           "[" + tid()     + "]" +
                           msg +
                           "\n";
    std::unique_lock<std::mutex> lock(mut_);
    // msgQueue_.push_back(std::move(logmsg));
    // if(msgQueue_.size() >= 100) {
    //     cv_.notify_one();
    // }
    msg_.append(logmsg);
    if(msg_.size() >= FLUSH_SIZE) {
        cv_.notify_one();
    }
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