#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <string>

std::string filepath = "/mnt/e/GitHub/miniduo_/workspace/../resource/index.txt";

int main() {
    struct stat filestat;
    if(stat(filepath.c_str(), &filestat) < 0) {
        printf("no such file\n");
        return 0;
    }
    printf("File length: %ld", filestat.st_size);
    return 0;
}