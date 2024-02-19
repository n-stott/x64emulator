#ifndef UTILS_H
#define UTILS_H

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