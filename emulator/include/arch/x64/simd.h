#ifndef SIMD_H
#define SIMD_H

#include "utils.h"

namespace x64 {

    enum class SIMD_ROUNDING {
        NEAREST,
        DOWN,
        UP,
        ZERO,
    };


    struct SimdControlStatus {
        SIMD_ROUNDING rc { SIMD_ROUNDING::NEAREST };

        u32 asDoubleWord() const;
        static SimdControlStatus fromDoubleWord(u32 mxcsr);
    };

}

#endif