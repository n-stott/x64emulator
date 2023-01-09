#include <cstddef>
#include <stdarg.h>
#include <locale.h>
#include <wchar.h>
#include "fmt/printf.h"

extern "C" {

    __attribute__((noinline))
    int intrinsic$putchar(int) { return 1; }

    __attribute__((noinline))
    void* intrinsic$malloc(size_t) { return nullptr; }

    int fakelibc$putchar(int c) {
        return intrinsic$putchar(c);
    }

    FILE* fakelibc$stdout = (FILE*)0x57D0117;

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

    void* fakelibc$memset(void* s, int c, size_t n) {
        unsigned char* dst = (unsigned char*)s;
        while(n > 0) {
            *dst = c;
            --n;
        }
        return s;
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

    size_t fakelibc$fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
        (void)stream;
        if(size != 1) return 0;
        for(size_t i = 0; i < nmemb; ++i) {
            intrinsic$putchar(((const char*)ptr)[i]);
        }
        return nmemb;
    }

    int fakelibc$__cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
        return 0;
    }

    alignas(64) static  __locale_struct c_locale { 0 };

    locale_t fakelibc$__newlocale(int category_mask, const char *locale, locale_t base) {
        return &c_locale;
    }

    locale_t fakelibc$__uselocale(locale_t newloc) {
        return newloc;
    }

    int fakelibc$wctob(wint_t c) {
        return (int)c;
    }

    wint_t fakelibc$btowc(int c) {
        return (wint_t)c;
    }

    wctype_t fakelibc$__wctype_l(const char* name, locale_t locale) {
        return 0;
    }

    int fakelibc$strcmp(const char* s1, const char* s2) {
        while(*s1 && *s2) {
            unsigned char a = *s1++;
            unsigned char b = *s2++;
            if(a < b) return -1;
            if(a > b) return +1;
        }
        return 0;
    }

    int fakelibc$fputs(const char* __restrict__ s, FILE* __restrict__ stream) {
        int count = 0;
        while(*s) count += fakelibc$putchar(*s++);
        return count;
    }

    int fakelibc$fflush(FILE* stream) {
        return 0;
    }

    int fakelibc$fputc(int c, FILE* stream) {
        return fakelibc$putchar(c);
    }

    time_t time(time_t* tloc) {
        return 0;
    }

}