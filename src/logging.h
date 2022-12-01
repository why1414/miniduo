#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#define log(level, levelstr, format, args...)    \
    do {                   \
        Logger::getLogger().logv(level,  levelstr "[%s:%d][%s]: " format, __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while(0) 


#define log_trace(...) log(Logger::LogLevel::TRACE, "[TRACE]", __VA_ARGS__)      
#define log_debug(...) log(Logger::LogLevel::DEBUG, "[DEBUG]", __VA_ARGS__)      
#define log_info(...) log(Logger::LogLevel::INFO, "[INFO ]", __VA_ARGS__)        
#define log_warn(...) log(Logger::LogLevel::WARN, "[warn ]", __VA_ARGS__)        
#define log_error(...) log(Logger::LogLevel::ERROR, "[ERROR]", __VA_ARGS__)      
#define log_fatal(...) log(Logger::LogLevel::FATAL, "[FATAL]", __VA_ARGS__)   

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

public:
    enum class LogLevel { ALL = 0, TRACE, DEBUG, INFO, 
                        WARN, ERROR, FATAL };
    ~Logger();

    static Logger& getLogger();
    void logv(const LogLevel level, 
              const char* fmt, ...);
    
    void setLogLevel(LogLevel level);
    void setFileBaseName(std::string basename);
    void setRotateInterval(long interval);

private:
    // Logger();
    // 构造函数初始化参数
    Logger(LogLevel level,
           std::string fileBaseName,
           long rotateInterval);
    
    // 尝试切换log文件，只在后台线程中被调用
    void tryRotate(); 
    
    // 后台线程执行，把 消息队列flush

    void process();
    // 生成时间信息string
    std::string strTime() ;

private:
    std::mutex mut_;
    std::condition_variable cv_;
    std::queue<std::string> msgQueue_;
    std::thread backendThread_;
    std::atomic<bool> stopLogging_;
    // 存储每个log level 对应的string
    // static const std::string levelStrs_[7]; 
    std::string fileBaseName_;
    int fd_;
    LogLevel level_; /// FIXME: atomic
    long lastRotate_;
    long rotateInterval_;

};