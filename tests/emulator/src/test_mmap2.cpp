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
    // This should not crash
    char* access = (char*)ptr + size;
    char value1 = *access;
}