#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char* name = ptsname(0);
    if(name) {
        puts(name);
    } else {
        perror("ptsname");
    }

    name = ttyname(0);
    if(name) {
        puts(name);
    } else {
        perror("ttyname");
    }

    name = ttyname(1);
    if(name) {
        puts(name);
    } else {
        perror("ttyname");
    }

    name = ttyname(2);
    if(name) {
        puts(name);
    } else {
        perror("ttyname");
    }

    int rc = fcntl(0, F_GETFD, 0);
    if(rc < 0) {
        perror("fcntl(0, getfd)");
        return 1;
    }
    printf("stdin is cloexec: %d\n", rc);

    int fd = open("/dev/tty", O_CLOEXEC);
    if(fd < 0) {
        perror("open(/dev/tty, O_CLOEXEC)");
        return 1;
    }

    rc = fcntl(fd, F_GETFD, 0);
    if(rc < 0) {
        perror("fcntl(0, getfd)");
        return 1;
    }
    printf("fd is cloexec: %d\n", rc);
    return 0;
}