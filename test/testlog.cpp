#include "logging.h"

#include <thread>
#include <vector>
#include <iostream>

using namespace std;
void func(int id) {
    string s = "test";
    
    // Logger::getLogger().logv(Logger::LogLevel(id), "%d", id);
    // log(Logger::LogLevel(5), "sfda", "%d", id);
    // log(Logger::LogLevel(5), "sfda", "sdfad");
    log_trace("worker thread %d", id);
    // log_debug("test test");
    // cout<<id<<endl;
    exit_if(id == 1, "error, process exit");
}


int main() {
    // Logger::getLogger().setLogLevel(Logger::LogLevel::ERROR);
    vector<thread> ts;
    for(int i=0; i<20; i++) {
        ts.push_back(thread(func, i));
    }
    for(auto& t: ts)
        t.join();

    
    // std::cout<<__FILE__<<__LINE__<<std::endl;
    return 0;

}
