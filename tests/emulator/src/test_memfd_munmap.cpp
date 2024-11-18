#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    // Sys::memfd_create(name=wayland-cursor, flags=0x3) = 10
    int fd = ::memfd_create("mytmpfile", MFD_CLOEXEC);
    if(fd < 0) {
        perror("memfd_create");
        return 1;
    }

    const size_t size1 = 0x900;
    const size_t size2 = 0x1b00;

    // Sys::fallocate(fd=10, mode=0, offset=0x0, len=2304) = 0
    if(::fallocate(fd, 0, 0, size1) < 0) {
        perror("fallocate");
        return 1;
    }

    // Sys::mmap(addr=0x0, length=2304, prot=RW, flags=0x2, fd=10, offset=0) = 0x522e000
    void* ptr1 = ::mmap(nullptr, size1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(ptr1 == (void*)MAP_FAILED) {
        perror("mmap 1");
        return 1;
    }

    // Sys::ftruncate(fd=10, length=6912) = 0
    if(::ftruncate(fd, size2) < 0) {
        perror("ftruncate");
        return 1;
    }

    // Sys::fallocate(fd=10, mode=0, offset=0x0, len=6912) = 0
    if(::fallocate(fd, 0, 0, size2) < 0) {
        perror("fallocate");
        return 1;
    }

    // Sys::munmap(addr=0x522e000, length=2304) = 0
    if(::munmap(ptr1, size1) < 0) {
        perror("munmap");
    }

    // Sys::mmap(addr=0x0, length=2304, prot=RW, flags=0x22, fd=0, offset=0) = 0x522f000
    void* garbage_ptr = ::mmap(nullptr, size1, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);

    // Sys::mmap(addr=0x0, length=6912, prot=RW, flags=0x2, fd=10, offset=0) = 0x522f000
    void* ptr2 = ::mmap(nullptr, size2, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(ptr2 == (void*)MAP_FAILED) {
        perror("mmap 1");
        return 1;
    }

}