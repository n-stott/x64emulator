#include <cstddef>
#include <stdarg.h>
#include <fmt/printf.h>

extern "C" {

    __attribute__((noinline))
    int intrinsic$putchar(int) { return 1; }

    __attribute__((noinline))
    void* intrinsic$malloc(size_t) { return nullptr; }

    int fakelibc$putchar(int c) {
        return intrinsic$putchar(c);
    }

    int fakelibc$puts(const char* s) {
        if(!s) {
            return intrinsic$putchar('$');
        }
        int nbytes = 0;
        while(*s) {
            nbytes += intrinsic$putchar(*s);
            ++s;
        }
        nbytes += intrinsic$putchar('\n');
        return nbytes;
    }

    void* fakelibc$memchr(const void* s, int c, int n) {
        unsigned char* ptr = (unsigned char*)s;
        int count = 0;
        while(count < n) {
            if(*ptr == c) return ptr;
            ++ptr;
            ++count;
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

    void fakelibc$free(void* ptr) {
        (void)ptr;
    }

    void* fakelibc$memcpy(void* dest, const void* src, size_t n) {
        unsigned char* d = (unsigned char*)dest;
        const unsigned char* s = (const unsigned char*)src;
        while(n > 0) {
            *d++ = *s++;
            --n;
        }
        return dest;
    }

    void* fakelibc$memmove(void* dest, const void* src, size_t n) {
        unsigned char* d = (unsigned char*)dest;
        const unsigned char* s = (const unsigned char*)src;
        while(n > 0) {
            *d++ = *s++;
            --n;
        }
        return dest;
    }

    int fakelibc$memcmp(const void* s1, const void* s2, size_t n) {
        const unsigned char* src1 = (const unsigned char*)s1;
        const unsigned char* src2 = (const unsigned char*)s2;
        while(n > 0) {
            unsigned char a = *src1++;
            unsigned char b = *src2++;
            if(a < b) return -1;
            if(a > b) return +1;
            --n;
        }
        return 0;
    }

    int fakelibc$vsnprintf(char* str, size_t size, const char* format, va_list ap) {
        if(!size) return 0;
        int retsize = 1;
        if(size >= 2) {
            str[0] = 'e';
            str[1] = '\0';
            retsize = 1;
        }
        if(size >= 3) {
            str[1] = 'r';
            str[2] = '\0';
            retsize = 2;
        }
        if(size >= 4) {
            str[2] = 'r';
            str[3] = '\0';
            retsize = 3;
        }
        return retsize;
    }

}