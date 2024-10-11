#include <cstring>
#include <sys/mman.h>
#include <stdio.h>

int test1() {
    void* ptr = mmap(nullptr, 0x2000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(!ptr) {
        puts("mmap");
        return 1;
    }
    munmap(ptr, 0x2000);
    char* region1 = (char*)mmap(ptr, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    char* region2 = (char*)mmap(region1 + 0x1000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(region2 != region1 + 0x1000) {
        puts("allocation");
        return 1;
    }

    memset(region1, 0x1, 0x1000);
    memset(region2, 0x2, 0x1000);

    size_t size = 0x14;
    memmove(region2 + 0x1000 - size, region1, size);
    if(region2[0x1000-size] != 0x1) {
        puts("memmove");
        return 1;
    }

    return 0;
}

int test2() {
    void* ptr = mmap(nullptr, 0x3000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(!ptr) {
        puts("mmap");
        return 1;
    }
    munmap(ptr, 0x3000);
    char* region1 = (char*)mmap(ptr, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    char* region2 = (char*)mmap(region1 + 0x1000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    char* region3 = (char*)mmap(region2 + 0x1000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(region2 != region1 + 0x1000) {
        puts("allocation");
        return 1;
    }
    if(region3 != region2 + 0x1000) {
        puts("allocation");
        return 1;
    }

    memset(region1, 0x1, 0x1000);
    memset(region2, 0x2, 0x1000);
    memset(region3, 0x3, 0x1000);

    for(size_t size = 1; size < 64; ++size) {
        for(size_t offset = 1; offset < 256 && offset < size; ++offset) {
            memmove(region3, region2 - offset, size);
            if(region3[offset] == region3[offset-1]) {
                printf("size=%zu offset=%zu\n", size, offset);
                for(size_t i = 0; i < size+2; ++i) {
                    printf("region3[%zu] = %hhd\n", i, region3[i]);
                }
                puts("memmove");
                return 1;
            }
            memset(region3, 0x3, 0x1000);
        }
    }

    return 0;
}

int main() {
    int rc = 0;
    rc |= test1();
    rc |= test2();
    return rc;
}