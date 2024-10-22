#include <sys/mman.h>
#include <stdio.h>

int main() {
    void* base = mmap(nullptr, 0x2000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if(base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    if(munmap(base, 0x2000) < 0) {
        perror("munmap");
        return 1;
    }
    void* p1 = mmap(base, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if(p1 == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    if(p1 != base) {
        puts("Different base");
        return 1;
    }
    void* p2 = mmap((char*)p1 + 0x1000, 0x1000, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if(p2 == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    if(munmap(base, 0x2000) < 0) {
        perror("munmap");
        return 1;
    }
    return 0;
}