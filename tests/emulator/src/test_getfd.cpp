#include <fcntl.h>
#include <stdio.h>

int main() {
    int rc = fcntl(0, F_GETFD, 0);
    if(rc < 0) {
        perror("fcntl(0, getfd)");
        return 1;
    }
    printf("stdin is cloexec: %d\n", rc);
    return 0;
}