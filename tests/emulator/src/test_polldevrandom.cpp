#include <fcntl.h>
#include <poll.h>
#include <stdio.h>

int main() {
    int fd = open("/dev/random", O_CLOEXEC | O_RDONLY);
    if(fd < 0) {
        perror("open");
        return 1;
    }
    pollfd fds { fd, POLLIN, 0 };
    int ret = poll(&fds, 1, -1);
    if(ret < 0) {
        perror("poll");
        return 1;
    }
    return 0;
}