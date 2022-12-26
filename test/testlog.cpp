#include "miniduo/logging.h"

#include <thread>
#include <vector>
#include <iostream>

using namespace std;
void func(int id) {
    string s = "test";
    
    // Logger::getLogger().logv(Logger::LogLevel(id), "%d", id);
    // log(Logger::LogLevel(5), "sfda", "%d", id);
    // log(Logger::LogLevel(5), "sfda", "sdfad");
    streamlog_trace << "streamlog worker thread " << id ;
    log_trace("worker thread %d", id);
    // Logger::getLogger().streamLogv() << "[StreamLog]" << "[" <<__FILE__ << ":" << __LINE__  << "]" << "[" << __FUNCTION__ << "]: " << "worker thread " << id ;
    // log_debug("test test");
    // cout<<id<<endl;
    // exit_if(id == 1, "error, process exit");
}


int main() {
    // Logger::getLogger().setLogLevel(Logger::LogLevel::ERROR);
    vector<thread> ts;
    // set_logLevel(Logger::LogLevel::DISABLE);
    // set_logSwitchToFileLog();
    for(int i=0; i<20; i++) {
        ts.push_back(thread(func, i));
    }
    for(auto& t: ts)
        t.join();

    return 0;

}
