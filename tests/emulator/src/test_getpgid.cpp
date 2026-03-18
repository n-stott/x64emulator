#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = ::getpid();
    pid_t tid = ::gettid();
    if(tid != pid) return 1;
    pid_t pgid = ::getpgid(pid);
    if(pgid != pid) return 1;
    if(pgid != ::getpgid(0)) return 1;
    printf("pid=%d  tid=%d  pgid=%d\n", pid, tid, pgid);
    return 0;
}