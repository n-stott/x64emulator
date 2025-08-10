#include <sys/eventfd.h>
#include <unistd.h>
#include <cstdio>

int main() {
    const size_t runs = 10'000;
    for(size_t i = 0; i < runs; ++i) {
        int fd = ::eventfd(i, EFD_CLOEXEC);
        if(fd < 0) {
            perror("eventfd");
            return 1;
        }
        if(::close(fd) < 0) {
            perror("close fd");
        }
    }
    return 0;
}