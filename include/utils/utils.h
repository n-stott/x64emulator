#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <cstring>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

struct Xmm {
    u64 lo;
    u64 hi;
};
using u128 = Xmm;

using f32 = float;
using f64 = double;

struct F80 {
    u8 val[10];

    static F80 fromLongDouble(long double d) {
        F80 f;
        ::memcpy(&f, &d, sizeof(f));
        return f;
    }

    static long double toLongDouble(F80 f) {
        long double d;
        ::memset(&d, 0, sizeof(d));
        ::memcpy(&d, &f, sizeof(f));
        return d;
    }
};
using f80 = F80;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(u128) == 16);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
static_assert(sizeof(f80) == 10);
static_assert(sizeof(long double) == 16);

#endif