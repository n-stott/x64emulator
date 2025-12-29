#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    int fd[2];
    if(::pipe2(fd, O_NONBLOCK | O_CLOEXEC) < 0) {
        perror("pipe");
        return 1;
    }

    int readFd = fd[0];
    int writeFd = fd[1];
    printf("readfd=%d writefd=%d\n", readFd, writeFd);

    const char* message = "Hello there !\n";

    if(::write(writeFd, message, strlen(message)) < 0) {
        perror("write");
        return 1;
    }

    char buf[128];
    if(::read(readFd, buf, sizeof(buf)) < 0) {
        perror("read");
        return 1;
    }

    printf(buf);

    int ret = ::read(readFd, buf, sizeof(buf));
    if(ret >= 0) {
        perror("read second time");
        return 1;
    }
    if(errno != EAGAIN) {
        perror("read second time");
        return 1;
    }

    if(::close(writeFd) < 0) {
        perror("write");
        return 1;
    }

    ret = ::read(readFd, buf, sizeof(buf));
    if(ret != 0) {
        perror("read third time");
        return 1;
    }

    return 0;
}