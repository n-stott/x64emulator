#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    int fd = ::open("/dev/tty", O_CLOEXEC | O_RDWR);
    if(fd < 0) {
        ::perror("open /dev/tty");
        return 1;
    }
    if(::close(STDOUT_FILENO) < 0) {
        ::perror("close stdout");
        return 1;
    }
    if(::close(STDERR_FILENO) < 0) {
        ::perror("close stderr");
        return 1;
    }

    if(::lseek(fd, 10, SEEK_SET) < 0) {
        assert(errno == ESPIPE);
        ::perror("lseek /dev/tty");
    }
    
    const char* message = "Hello there !\n";
    if(::write(fd, message, ::strlen(message)) < 0) {
        ::perror("write /dev/tty");
        return 1;
    }

    if(::close(STDIN_FILENO) < 0) {
        ::perror("close stdin");
        return 1;
    }

    if(::close(fd) < 0) {
        ::perror("close /dev/tty");
        return 1;
    }
    return 0;
}