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




Logger::Logger()
    : level_(LogLevel::TRACE),
      fileDir_("log/"),
      fileBaseName_("log"),
      rotateInterval_(24 * 60 * 60),
      lastRotate_(::time(nullptr)),
      fd_(-1),
      stopLogging_(false),
      isfilelog_(true),
      backendThread_(std::thread(&Logger::process, this))

{   
    ::mkdir(fileDir_.c_str(), 0);
    newLogfile();
    msgQueue_.push("[Logger created] \n");
}

Logger::~Logger() {
    {
        std::unique_lock<std::mutex> lock(mut_);
        stopLogging_ = true;
        msgQueue_.push("[Logger destroyed] \n");
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
        std::unique_lock<std::mutex> lock(mut_);

        while(msgQueue_.empty() && !stopLogging_) {
            cv_.wait(lock);
        }

        while(!msgQueue_.empty()) {
            std::string msg = msgQueue_.front();
            msgQueue_.pop();
            lock.unlock();
            // 写入文件
            int err = ::write(fd_, msg.c_str(), msg.size());
            lock.lock();
        }
             

    }

}

void Logger::switchToStdout() {
    if(fd_ > 0) {
        ::close(fd_);
    }
    fd_ = 1;
    isfilelog_ = false;
}

void Logger::newLogfile() {
    assert(lastRotate_ > 0);
    struct tm ntm;
    ::localtime_r(&lastRotate_, &ntm);
    char newname[4096];
    ::snprintf( newname, sizeof(newname), 
                "%s%s.%d%02d%02d%02d%02d", 
                fileDir_.c_str(), fileBaseName_.c_str(), 
                ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday, ntm.tm_hour, ntm.tm_min);
    fd_ = ::open(newname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
    if(fd_ < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", newname, ::strerror(errno));
        exit(1);
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
    time_t now = ::time(nullptr);
    if((now - lastRotate_) < rotateInterval_ ) {
        return ;
    }
    lastRotate_ = now;
    ::close(fd_);
    fd_ = -1;
    newLogfile();

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