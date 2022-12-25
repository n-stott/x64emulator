#include <cstddef>

extern "C" {

    __attribute__((noinline))
    void intrinsic$putchar(char) { }

    __attribute__((noinline))
    void* intrinsic$malloc(size_t) { return nullptr; }

    int fakelibc$putchar(char c) {
        intrinsic$putchar(c);
        return 0;
    }

    int fakelibc$puts(const char* s) {
        while(*s) {
            intrinsic$putchar(*s);
            ++s;
        }
        intrinsic$putchar('\n');
        return 0;
    }

    void* fakelibc$memchr(const void* s, int c, int n) {
        unsigned char* ptr = (unsigned char*)s;
        int count = 0;
        while(count < n) {
            if(*ptr == c) return ptr;
            ++ptr;
        }
        return nullptr;
    }

    size_t fakelibc$strlen(const char* s) {
        size_t len = 0;
        while(*s) {
            ++s;
            ++len;
        }
        return len;
    }

    void* fakelibc$malloc(size_t size) {
        void* ptr = intrinsic$malloc(size);
        return ptr;
    }

}