#include "util.h"
#include <unistd.h> // pit_t, ::gettid()


using namespace miniduo;

__thread pid_t t_cachedTid = 0;

pid_t util::currentTid(){
    if(t_cachedTid == 0){
        t_cachedTid = ::gettid();
    }
    return t_cachedTid;
}