#include <unistd.h>
#include <cstdio>

int main() {
    const size_t runs = 10'000;
    for(size_t i = 0; i < runs; ++i) {
        int fds[2];
        if(::pipe(fds) < 0) {
            perror("pipe");
            return 1;
        }
        if(::close(fds[0]) < 0) {
            perror("close fds[0]");
        }
        if(::close(fds[1]) < 0) {
            perror("close fds[1]");
        }
    }
    return 0;
}