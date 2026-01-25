#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/wait.h>

int main() {
    printf("Parent process has pid=%d\n", getpid());
    pid_t pid1 = fork();
    if(pid1 < 0) {
        perror("fork");
        return 1;
    }
    if(pid1 == 0) {
        printf("Hello from new process pid=%d\n", getpid());
        exit(0);
    }

    printf("Hello from old process pid=%d\n", getpid());
    pid_t pid2 = fork();
    if(pid2 < 0) {
        perror("fork");
        return 1;
    }
    if(pid2 == 0) {
        printf("Hello from new process pid=%d\n", getpid());
        exit(0);
    }

    pid_t wpid1 = wait(nullptr);
    printf("wait provided pid=%d\n", wpid1);
    pid_t wpid2 = wait(nullptr);
    printf("wait provided pid=%d\n", wpid2);
    pid_t wpid3 = wait(nullptr);
    printf("wait provided pid=%d (errno=%d)\n", wpid3, errno);
    if(wpid3 >= 0) return 1;
    if(errno != ECHILD) return 1;
    exit(0);
}