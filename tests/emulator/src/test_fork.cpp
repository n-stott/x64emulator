#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>

int main() {
    printf("Parent process has pid=%d\n", getpid());
    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return 1;
    }
    if(pid == 0) {
        printf("Hello from new process pid=%d\n", getpid());
        exit(0);
    } else {
        printf("Hello from old process pid=%d\n", getpid());
        waitpid(pid, nullptr, 0);
        exit(0);
    }
}