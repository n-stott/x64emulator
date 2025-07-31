#include "x64/types.h"
#include "host/hostinstructions.h"
#include <cmath>
#include <cstring>

namespace x64 {

    // LOL, std::round does not take rounding mode into account.
    // It rounds away from 0 for midpoints, instead of towards even.
    // We let the host do this work instead.
    i32 F32::round32(f32 val) {
        return host::roundWithoutTruncation32(val);
    }

    // LOL, std::round does not take rounding mode into account.
    // It rounds away from 0 for midpoints, instead of towards even.
    // We let the host do this work instead.
    i64 F32::round64(f32 val) {
        return host::roundWithoutTruncation64(val);
    }

    // LOL, std::round does not take rounding mode into account.
    // It rounds away from 0 for midpoints, instead of towards even.
    // We let the host do this work instead.
    i32 F64::round32(f64 val) {
        return host::roundWithoutTruncation32(val);
    }

    // LOL, std::round does not take rounding mode into account.
    // It rounds away from 0 for midpoints, instead of towards even.
    // We let the host do this work instead.
    i64 F64::round64(f64 val) {
        return host::roundWithoutTruncation64(val);
    }

    f80 F80::fromLongDouble(long double d) {
        f80 f;
        std::memcpy(&f, &d, sizeof(f));
        return f;
    }

    long double F80::toLongDouble(f80 f) {
        long double d;
        std::memset(&d, 0, sizeof(d));
        std::memcpy(&d, &f, sizeof(f));
        return d;
    }

    f80 F80::bitcastFromU32(u32 val) {
        f32 f;
        std::memcpy(&f, &val, sizeof(f));
        return fromLongDouble((long double)f);
    }

    f80 F80::bitcastFromU64(u64 val) {
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

    f80 F80::castFromI16(i16 val) {
        return fromLongDouble((long double)val);
    }

    f80 F80::castFromI32(i32 val) {
        return fromLongDouble((long double)val);
    }

    f80 F80::castFromI64(i64 val) {
        return fromLongDouble((long double)val);
    }

    i16 F80::castToI16(f80 val) {
        return (i16)toLongDouble(val);
    }

    i32 F80::castToI32(f80 val) {
        return (i32)toLongDouble(val);
    }

    i64 F80::castToI64(f80 val) {
        return (i64)toLongDouble(val);
    }

    // LOL, std::round does not take rounding mode into account.
    // It rounds away from 0 for midpoints, instead of towards even.
    // We let the host do this work instead.
    f80 F80::roundNearest(f80 val) {
        return host::round(val);
    }

    f80 F80::roundDown(f80 val) {
        return fromLongDouble(std::floor(toLongDouble(val)));
    }

    f80 F80::roundUp(f80 val) {
        return fromLongDouble(std::ceil(toLongDouble(val)));
    }

    f80 F80::roundZero(f80 val) {
        return fromLongDouble(std::trunc(toLongDouble(val)));
    }

}