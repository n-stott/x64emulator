#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    int fd = ::memfd_create("my_tmp_file", MFD_CLOEXEC);
    if(fd < 0) {
        perror("memfd_create");
        return 1;
    }

    if(::ftruncate(fd, 0x1000) < 0) {
        perror("ftruncate");
        return 1;
    }

    char* ptr = (char*)::mmap(nullptr, 0x1000, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(ptr == (void*)MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if(::close(fd) < 0) {
        perror("close");
        return 1;
    }

    *ptr = 1;

    return 0;
}