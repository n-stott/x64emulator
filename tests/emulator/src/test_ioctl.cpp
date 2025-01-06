#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <cassert>
#include <cstdio>

int main() {
    int fd = 2;
    int opt = 0;
    int ret = ::ioctl(fd, FIONBIO, &opt);
    if(ret < 0) {
        perror("ioctl");
        return 1;
    }
    int fl = ::fcntl(fd, F_GETFL, 0);
    assert(fl >= 0);
    if(ret < 0) {
        perror("fcntl");
        return 1;
    }
    if((fl & O_NONBLOCK) != 0) {
        printf("fgetfl=%d\n", fl);
        puts("nonblock but not supposed to be");
        return 1;
    }
    opt = 1;
    ret = ::ioctl(fd, FIONBIO, &opt);
    if(ret < 0) {
        perror("ioctl");
        return 1;
    }
    fl = ::fcntl(fd, F_GETFL, 0);
    assert(fl >= 0);
    if(ret < 0) {
        perror("fcntl");
        return 1;
    }
    if((fl & O_NONBLOCK) == 0) {
        printf("fgetfl=%d\n", fl);
        puts("not nonblock");
        return 1;
    }
    
    return 0;
}