#include "interpreter/simd.h"

namespace x64 {

    u32 SimdControlStatus::asDoubleWord() const {
        u32 mxcsr = ((u32)rc << 13);
        return mxcsr;
    }

    SimdControlStatus SimdControlStatus::fromDoubleWord(u32 mxcsr) {
        SimdControlStatus csr;
        csr.rc = (SIMD_ROUNDING)((mxcsr >> 13) & 0x3);
        return csr;
    }

}