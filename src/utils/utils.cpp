#include "utils/utils.h"
#include <cmath>
#include <cstring>

F80 F80::fromLongDouble(long double d) {
    F80 f;
    ::memcpy(&f, &d, sizeof(f));
    return f;
}

long double F80::toLongDouble(F80 f) {
    long double d;
    ::memset(&d, 0, sizeof(d));
    ::memcpy(&d, &f, sizeof(f));
    return d;
}

F80 F80::bitcastFromU32(u32 val) {
    f32 f;
    ::memcpy(&f, &val, sizeof(f));
    return fromLongDouble((long double)f);
}

F80 F80::bitcastFromU64(u64 val) {
    f64 d;
    ::memcpy(&d, &val, sizeof(d));
    return fromLongDouble((long double)d);
}

F80 F80::castFromI16(i16 val) {
    return fromLongDouble((long double)val);
}

F80 F80::castFromI32(i32 val) {
    return fromLongDouble((long double)val);
}

F80 F80::castFromI64(i64 val) {
    return fromLongDouble((long double)val);
}

i16 F80::castToI16(F80 val) {
    return (i16)toLongDouble(val);
}

i32 F80::castToI32(F80 val) {
    return (i32)toLongDouble(val);
}

i64 F80::castToI64(F80 val) {
    return (i64)toLongDouble(val);
}

// LOL, std::round does not take rounding mode into account.
// It rounds away from 0 for midpoints, instead of towards even.
// We let the host do this work instead.
F80 F80::roundNearest(F80 val) {
    // save control word
    long double x = toLongDouble(val);
    unsigned short cw { 0 };
    asm volatile("fnstcw %0" : "=m" (cw));

    // set rounding mode to nearest
    unsigned short nearest = (unsigned short)(cw & ~(0x3 << 10));
    asm volatile("fldcw %0" :: "m" (nearest));

    // do the rounding
    asm volatile("fldt %0" :: "m"(x));
    asm volatile("frndint");
    asm volatile("fstpt %0" : "=m"(x));

    // load initial control word
    asm volatile("fldcw %0" :: "m" (cw));

    return fromLongDouble(x);
}

F80 F80::roundDown(F80 val) {
    return fromLongDouble(std::floor(toLongDouble(val)));
}

F80 F80::roundUp(F80 val) {
    return fromLongDouble(std::ceil(toLongDouble(val)));
}

F80 F80::roundZero(F80 val) {
    return fromLongDouble(std::trunc(toLongDouble(val)));
}