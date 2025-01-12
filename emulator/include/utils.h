#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <cstring>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using Mmx = u64;

struct Xmm {
    u64 lo;
    u64 hi;
};
using u128 = Xmm;

inline bool operator==(u128 a, u128 b) {
    return a.lo == b.lo && a.hi == b.hi;
}

using f32 = float;
using f64 = double;

struct F32 {
    static i32 round32(f32 val);
    static i64 round64(f32 val);
};

struct F64 {
    static i64 round(f64 val);
};

struct F80 {
    u8 val[10];

    static F80 fromLongDouble(long double d);
    static long double toLongDouble(F80 f);
    static F80 bitcastFromU32(u32 val);
    static F80 bitcastFromU64(u64 val);
    static u32 bitcastToU32(F80 val);
    static u64 bitcastToU64(F80 val);
    static F80 castFromI16(i16 val);
    static F80 castFromI32(i32 val);
    static F80 castFromI64(i64 val);
    static i16 castToI16(F80 val);
    static i32 castToI32(F80 val);
    static i64 castToI64(F80 val);

    static F80 roundNearest(F80 val);
    static F80 roundDown(F80 val);
    static F80 roundUp(F80 val);
    static F80 roundZero(F80 val);
};
using f80 = F80;
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
static_assert(sizeof(long double) == 16);

#endif