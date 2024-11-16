#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = ::open("/dev/null", O_RDWR | O_CLOEXEC);
    if(fd < 0) {
        perror("open /dev/null");
        return 1;
    }

    int stderr_fd = ::dup(STDERR_FILENO);
    if(stderr_fd < 0) {
        perror("dup stdout");
        return 1;
    }

    if(::dup2(fd, STDERR_FILENO) < 0) {
        perror("dup2 install /dev/null");
        return 1;
    }

    fprintf(stderr, "Hello there ! You shouldn't be able to see me :)\n");

    if(::dup2(stderr_fd, STDERR_FILENO) < 0) {
        perror("dup2 restore stdout");
        return 1;
    }

    fprintf(stderr, "But now you can !\n");

    return 0;
}