#include <cstddef>
#include <stdarg.h>
#include <locale.h>
#include <wchar.h>
#include <pthread.h>
#include "fmt/printf.h"

extern "C" {

    __attribute__((noinline))
    extern int intrinsic$putchar(int);

    __attribute__((noinline))
    extern void* intrinsic$malloc(size_t);

    __attribute__((noinline))
    extern void intrinsic$free(void*);

    __attribute__((noinline))
    extern FILE* intrinsic$fopen64(const char*, const char*);

    __attribute__((noinline))
    extern int intrinsic$fileno(FILE*);

    __attribute__((noinline))
    extern ssize_t intrinsic$read(int, void*, size_t);

    __attribute__((noinline))
    extern int intrinsic$fclose(FILE*);

    int fakelibc$putchar(int c) {
        return intrinsic$putchar(c);
    }

    FILE* const fakelibc$stdin  = (FILE*)0x57D111;
    FILE* const fakelibc$stdout = (FILE*)0x57D0117;
    FILE* const fakelibc$stderr  = (FILE*)0x57DE44;

    locale_t const c_locale = (locale_t)0xC10CA1E;

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
        intrinsic$free(ptr);
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
        if(dest < src) {
            while(n > 0) {
                *d++ = *s++;
                --n;
            }
        } else if (src < dest) {
            d += n-1;
            s += n-1;
            while(n > 0) {
                *d-- = *s--;
                --n;
            }
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

    locale_t fakelibc$__newlocale(int category_mask, const char *locale, locale_t base) {
        return c_locale;
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

    time_t fakelibc$time(time_t* tloc) {
        static int secondsSinceEpoch = 0xeb0c;
        ++secondsSinceEpoch;
        return secondsSinceEpoch;
    }

    int fakelibc$pthread_once(pthread_once_t* once_control, void (*init_routine)(void)) {
        static bool called = false;
        if(!called) {
            init_routine();
            called = true;
        }
        return 0;
    }

    int fakelibc$__pthread_key_create(pthread_key_t* key, void (*destructor)(void*)) {
        return 0;
    }

    FILE* fakelibc$fopen64(const char* pathname, const char* mode) {
        return intrinsic$fopen64(pathname, mode);
    }

    int fclose(FILE* file) {
        return intrinsic$fclose(file);
    }

    int fakelibc$fileno(FILE* file) {
        return intrinsic$fileno(file);
    }

    ssize_t fakelibc$read(int fd, void* buf, size_t count) {
        return intrinsic$read(fd, buf, count);
    }

}
