#ifndef UTILS_H
#define UTILS_H

#include <cassert>
#include <cstdint>
#include <cstring>

#ifdef MSVC_COMPILER
#define UNREACHABLE() __assume(0)
#else
#define UNREACHABLE() __builtin_unreachable()
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

struct u128 {
    u64 lo;
    u64 hi;
};

inline bool operator==(u128 a, u128 b) {
    return a.lo == b.lo && a.hi == b.hi;
}

inline bool operator!=(u128 a, u128 b) {
    return !(a == b);
}

inline u128 operator<<(u128 a, int shift) {
    if(shift >= 128) {
        return u128 { 0, 0 };
    } else if(shift >= 64) {
        return u128 { 0, a.lo << (shift-64) };
    } else {
        u64 newlo = a.lo << shift;
        u64 newhi = (a.hi << shift) | (a.lo >> (64-shift));
        return u128 { newlo, newhi };
    }
}

inline u128 operator&(u128 a, u128 b) {
    return u128 { a.lo & b.lo, a.hi & b.hi };
}

inline u128 operator|(u128 a, u128 b) {
    return u128 { a.lo | b.lo, a.hi | b.hi };
}

inline u128 operator^(u128 a, u128 b) {
    return u128 { a.lo ^ b.lo, a.hi ^ b.hi };
}

using f32 = float;
using f64 = double;

struct f80 {
    u8 val[10];
};

inline bool operator==(f80 a, f80 b) {
    return memcmp(a.val, b.val, sizeof(f80)) == 0;
}

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

#ifndef MSVC_COMPILER
static_assert(sizeof(long double) == 16);
#endif

#endif