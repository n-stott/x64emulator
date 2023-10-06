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

    __attribute__((noinline))
    extern int intrinsic$atoi(const char*);

    int fakelibc$putchar(int c) {
        return intrinsic$putchar(c);
    }

    FILE* fakelibc$stdin  = (FILE*)0x57D111;
    FILE* fakelibc$stdout = (FILE*)0x57D0117;
    FILE* fakelibc$stderr  = (FILE*)0x57DE44;

    // copied from bits/types/__locale_t.h
    struct locale_struct {
        void* __locales[13]; // struct __locale_data*

        const unsigned short* __ctype_b { nullptr };
        const int* __ctype_tolower { nullptr };
        const int* __ctype_toupper { nullptr };

        const char* __names[13];

        locale_struct() {
            auto poor_memset = [](char* dst, int c, size_t n) {
                while(n--) {
                    *dst++ = c;
                }
            };
            poor_memset(reinterpret_cast<char*>(this), 0, sizeof(*this));
            __ctype_b = (const unsigned short*)0xc7b;
            __ctype_tolower = (const int*)0xc710;
            __ctype_toupper = (const int*)0xc711;
            __ctype_b = classic_table();
        }

        const unsigned short* classic_table() {
            using namespace std;
            static constexpr unsigned short classic_table[256] = {
                ctype_base::cntrl /* null */,
                ctype_base::cntrl /* ^A */,
                ctype_base::cntrl /* ^B */,
                ctype_base::cntrl /* ^C */,
                ctype_base::cntrl /* ^D */,
                ctype_base::cntrl /* ^E */,
                ctype_base::cntrl /* ^F */,
                ctype_base::cntrl /* ^G */,
                ctype_base::cntrl /* ^H */,
                ctype_base::mask(ctype_base::space | ctype_base::cntrl | ctype_base::blank) /* tab */,
                ctype_base::mask(ctype_base::space | ctype_base::cntrl) /* LF */,
                ctype_base::mask(ctype_base::space | ctype_base::cntrl) /* ^K */,
                ctype_base::mask(ctype_base::space | ctype_base::cntrl) /* FF */,
                ctype_base::mask(ctype_base::space | ctype_base::cntrl) /* ^M */,
                ctype_base::cntrl /* ^N */,
                ctype_base::cntrl /* ^O */,
                ctype_base::cntrl /* ^P */,
                ctype_base::cntrl /* ^Q */,
                ctype_base::cntrl /* ^R */,
                ctype_base::cntrl /* ^S */,
                ctype_base::cntrl /* ^T */,
                ctype_base::cntrl /* ^U */,
                ctype_base::cntrl /* ^V */,
                ctype_base::cntrl /* ^W */,
                ctype_base::cntrl /* ^X */,
                ctype_base::cntrl /* ^Y */,
                ctype_base::cntrl /* ^Z */,
                ctype_base::cntrl /* esc */,
                ctype_base::cntrl /* ^\ */,
                ctype_base::cntrl /* ^] */,
                ctype_base::cntrl /* ^^ */,
                ctype_base::cntrl /* ^_ */,
                ctype_base::mask(ctype_base::space | ctype_base::print | ctype_base::blank) /*   */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ! */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* " */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* # */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* $ */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* % */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* & */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ' */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ( */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ) */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* * */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* + */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* , */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* - */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* . */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* / */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 0 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 1 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 2 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 3 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 4 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 5 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 6 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 7 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 8 */,
                ctype_base::mask(ctype_base::digit | ctype_base::xdigit | ctype_base::print) /* 9 */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* : */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ; */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* < */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* = */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* > */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ? */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ! */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* A */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* B */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* C */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* D */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* E */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::xdigit | ctype_base::print) /* F */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* G */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* H */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* I */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* J */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* K */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* L */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* M */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* N */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* O */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* P */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* Q */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* R */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* S */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* T */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* U */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* V */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* W */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* X */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* Y */,
                ctype_base::mask(ctype_base::alpha | ctype_base::upper | ctype_base::print) /* Z */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* [ */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* \ */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ] */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ^ */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* _ */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ` */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* a */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* b */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* c */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* d */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* e */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::xdigit | ctype_base::print) /* f */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* g */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* h */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* i */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* j */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* k */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* l */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* m */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* n */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* o */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* p */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* q */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* r */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* s */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* t */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* u */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* v */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* w */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* x */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* y */,
                ctype_base::mask(ctype_base::alpha | ctype_base::lower | ctype_base::print) /* x */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* { */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* | */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* } */,
                ctype_base::mask(ctype_base::punct | ctype_base::print) /* ~ */,
                ctype_base::cntrl /* del (0x7f)*/,
                /* The next 128 entries are all 0.   */
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };
            return classic_table;
        }
    };

    const locale_struct c_locale_data;

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
            ++dst;
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
        return (locale_t)&c_locale_data;
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
        while(*s) count += intrinsic$putchar(*s++);
        return count;
    }

    int fakelibc$fflush(FILE* stream) {
        return 0;
    }

    int fakelibc$fputc(int c, FILE* stream) {
        return intrinsic$putchar(c);
    }

    int fakelibc$putc(int c, FILE* stream) {
        return intrinsic$putchar(c);
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

    int fakelibc$atoi(const char* nptr) {
        return intrinsic$atoi(nptr);
    }

}
