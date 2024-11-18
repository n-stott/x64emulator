#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>

int main() {
    const size_t size = 0x900;

    void* ptr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
    if(ptr == (void*)MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Check that the full page was allocated
    char* access = (char*)ptr + size;
    char value1 = *access;

    if(::munmap(ptr, size) < 0) {
        perror("munmap");
        return 1;
    }

    if(::mprotect(ptr, size, PROT_READ) == 0) {
        puts("Cannot mproctect inexistant range");
        return 1;
    }
    if(errno != ENOMEM) {
        puts("mprotect should have generated ENOMEM");
        return 1;
    }
}