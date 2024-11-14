#include "utils.h"
#include "kernel/host.h"
#include <cmath>
#include <cstring>

// LOL, std::round does not take rounding mode into account.
// It rounds away from 0 for midpoints, instead of towards even.
// We let the host do this work instead.
i64 F64::round(f64 val) {
    return kernel::Host::roundWithoutTruncation(val);
}

F80 F80::fromLongDouble(long double d) {
    F80 f;
    std::memcpy(&f, &d, sizeof(f));
    return f;
}

long double F80::toLongDouble(F80 f) {
    long double d;
    std::memset(&d, 0, sizeof(d));
    std::memcpy(&d, &f, sizeof(f));
    return d;
}

F80 F80::bitcastFromU32(u32 val) {
    f32 f;
    std::memcpy(&f, &val, sizeof(f));
    return fromLongDouble((long double)f);
}

F80 F80::bitcastFromU64(u64 val) {
    f64 d;
    std::memcpy(&d, &val, sizeof(d));
    return fromLongDouble((long double)d);
}

u32 F80::bitcastToU32(f80 val) {
    f32 f = (f32)toLongDouble(val);
    u32 u;
    std::memcpy(&u, &f, sizeof(f));
    return u;
}

u64 F80::bitcastToU64(f80 val) {
    f64 f = (f64)toLongDouble(val);
    u64 u;
    std::memcpy(&u, &f, sizeof(f));
    return u;
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
    return kernel::Host::round(val);
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