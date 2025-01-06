#include <sys/mman.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

// void f(int* ptr) { *ptr = 1; }
using f_t = void(*)(int*);
const std::array<unsigned char, 20> f {{
    0x55,
    0x48, 0x89, 0xe5,
    0x48, 0x89, 0x7d, 0xf8,
    0x48, 0x8b, 0x45, 0xf8,
    0xc7, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x5d,
    0xc3
}};

// void inc(int* ptr) { ++(*ptr); }
using inc_t = void(*)(int*);
const std::array<unsigned char, 21> inc {{
    0x55,
    0x48, 0x89, 0xe5,
    0x48, 0x89, 0x7d, 0xf8,
    0x48, 0x8b, 0x45, 0xf8,
    0x8b, 0x08,
    0x83, 0xc1, 0x01,
    0x89, 0x08,
    0x5d,
    0xc3
}};

int main() {
    char* ptr = (char*)::mmap(nullptr, 0x1000, PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    assert(!!ptr);
    assert(ptr != (char*)MAP_FAILED);

    memcpy(ptr, f.data(), f.size());
    ::mprotect(ptr, 0x1000, PROT_READ|PROT_EXEC);
    f_t f = (f_t)ptr;

    int value = 0;
    f(&value);
    if(value != 1) {
        puts("f did not work");
        printf("value = %d\n", value);
        return 1;
    }

    ::mprotect(ptr, 0x1000, PROT_WRITE);

    memcpy(ptr, inc.data(), inc.size());
    ::mprotect(ptr, 0x1000, PROT_READ|PROT_EXEC);
    inc_t inc = (inc_t)ptr;

    inc(&value);
    if(value != 2) {
        puts("inc did not work");
        return 1;
    }
}