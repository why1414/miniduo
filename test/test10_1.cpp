#include "miniduo/EventLoop.h"


using namespace miniduo;
using namespace std;

class test {
    vector<EventLoop> loops;
public:
    test() :loops(10){}
};

int main() {
    // vector<EventLoop> loop(8);
    printf("------------------\n");
    EventLoops loops(0);
    // test t;

    // loop.loop();
    return 0;
}