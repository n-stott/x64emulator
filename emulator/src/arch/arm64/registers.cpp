#include "arch/arm64/registers.h"

namespace arm64 {

    Registers::Registers() {
        std::fill(gpr_.begin(), gpr_.end(), (u64)0);
        std::fill(simd_.begin(), simd_.end(), u128{0, 0});
    }

}