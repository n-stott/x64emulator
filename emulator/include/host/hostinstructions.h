#ifndef HOSTINSTRUCTIONS_H
#define HOSTINSTRUCTIONS_H

#include "utils.h"

namespace host {

    i32 roundWithoutTruncation32(f32 src);
    i64 roundWithoutTruncation64(f32 src);
    i32 roundWithoutTruncation32(f64 src);
    i64 roundWithoutTruncation64(f64 src);
    f80 round(f80);

    template<typename U>
    struct IdivResult {
        U quotient;
        U remainder;
    };

    IdivResult<u32> idiv32(u32 upperDividend, u32 lowerDividend, u32 divisor);
    IdivResult<u64> idiv64(u64 upperDividend, u64 lowerDividend, u64 divisor);


    template<typename U>
    struct ImulResult {
        U lower;
        U upper;
        bool carry;
        bool overflow;
    };

    ImulResult<u64> imul64(u64 a, u64 b);


    struct CPUID {
        u32 a, b, c, d;
    };
    CPUID cpuid(u32 a, u32 c);

    struct XGETBV {
        u32 a, d;
    };
    XGETBV xgetbv(u32 c);

}

#endif