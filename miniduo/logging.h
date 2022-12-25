#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <atomic>
#include <sstream>

// 格式化log
#define log(level, levelstr, format, args...)    \
    do {                   \
        if( level >= Logger::getLogger().getLogLevel()) {   \
            Logger::getLogger().logv(level,  levelstr "[%s:%d][%s]: " format, __FILE__, __LINE__, __FUNCTION__, ##args); \
        }   \
    } while(0) 


#define log_trace(...) log(Logger::LogLevel::TRACE, "[TRACE]", __VA_ARGS__)      
#define log_debug(...) log(Logger::LogLevel::DEBUG, "[DEBUG]", __VA_ARGS__)      
#define log_info(...)  log(Logger::LogLevel::INFO , "[INFO ]", __VA_ARGS__)        
#define log_warn(...)  log(Logger::LogLevel::WARN , "[WARN ]", __VA_ARGS__)        
#define log_error(...) log(Logger::LogLevel::ERROR, "[ERROR]", __VA_ARGS__)      
#define log_fatal(...) log(Logger::LogLevel::FATAL, "[FATAL]", __VA_ARGS__)   

// 流输出log
#define streamlog(level, levelstr) \
    Logger::getLogger().streamLogv(level)<<levelstr<<"["<<__FILE__<<":"<<__LINE__<<"]"<<"["<< __FUNCTION__ <<"]: "

#define streamlog_trace streamlog(Logger::LogLevel::TRACE, "[TRACE]")
#define streamlog_debug streamlog(Logger::LogLevel::DEBUG, "[DEBUG]")
#define streamlog_info  streamlog(Logger::LogLevel::INFO , "[INFO ]")
#define streamlog_warn  streamlog(Logger::LogLevel::WARN , "[WARN ]")
#define streamlog_error streamlog(Logger::LogLevel::ERROR, "[ERROR]")
#define streamlog_fatal streamlog(Logger::LogLevel::FATAL, "[FATAL]")


/* exit 结束前析构 logger，确保日志都flush*/
#define exit_if(b, ...)          \
    do {                                 \
        if(b) {                          \
            log(Logger::LogLevel::ERROR, "[ERROR]",  __VA_ARGS__) ;      \
            Logger::getLogger().~Logger();  \
            _Exit(1);             \
        }                                \
    } while(0)

#define set_logLevel(l) Logger::getLogger().setLogLevel(l)
#define set_logName(n)  Logger::getLogger().setFileBaseName(n)
#define set_logInterval(i) Logger::getLogger().setRotateInterval(i)
#define set_logSwitchToFileLog() Logger::getLogger().switchToFileLog()

static std::string _CutParenthesesNTail(std::string&& prettyFunction) {
    auto pos = prettyFunction.find('(');
    if(pos != std::string::npos) {
        prettyFunction.erase(prettyFunction.begin()+pos, prettyFunction.end());
    }
    return std::move(prettyFunction);
}

#define __STR_FUNCTION__ _CutParenthesesNTail(std::string(__PRETTY_FUNCTION__)).c_str()


class Logger {

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
private:
    class LogStream;

public:
    enum class LogLevel { ALL = 0, TRACE, DEBUG, INFO, 
                        WARN, ERROR, FATAL, DISABLE };
    ~Logger();

    static Logger& getLogger();
    void logv(const LogLevel level, 
              const char* fmt, ...);
    LogStream streamLogv(LogLevel level);

    void switchToFileLog();
    void setLogLevel(LogLevel level);
    void setFileBaseName(std::string basename);
    void setRotateInterval(long interval);
    LogLevel getLogLevel() const {
        return level_;
    }

private:
   
    // 构造函数初始化参数
    Logger();
    
    // 尝试切换log文件，只在后台线程中被调用
    void tryRotate(); 

    // 新建一个logfile，根据lastRotate_时间
    int openNewLogfile();
    
    // 后台线程执行，把 消息队列flush进磁盘
    void process();

    // 生成时间信息string
    std::string timeNow() ;

    // 生成线程id 
    std::string tid();

    // lock && flush
    void flush(const std::string &msg);

    void setfd(const int fd) {
        fd_ = fd;
    }

    void setfilelog(bool enable) {
        isfilelog_ = enable;
    }

private:
    std::mutex mut_;
    std::condition_variable cv_;
    std::list<std::string> msgQueue_; // replaced by msg_
    std::string msg_;
    // std::queue<std::string> msgOutputQueue_;

    std::thread backendThread_;
    std::atomic<bool> stopLogging_;
    std::atomic<bool> isfilelog_; // true: file log; false: stdout log
    std::string fileBaseName_;
    std::string fileDir_;
    int fd_;
    LogLevel level_; /// FIXME: atomic
    time_t lastRotate_;
    long rotateInterval_;

};

class Logger::LogStream: public std::ostringstream {
public:
    LogStream(Logger& logger, LogLevel level): logger_(logger), level_(level) {}
    // 拷贝构造函数
    LogStream(const LogStream &ls): logger_(ls.logger_), level_(ls.level_) {}
    ~LogStream() {
        if(level_ >= logger_.getLogLevel()) {
            logger_.flush(str());
        }
    }
private:
    Logger& logger_;
    LogLevel level_;
};