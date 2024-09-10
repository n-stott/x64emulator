#include "x64/registers.h"
#include <algorithm>

namespace x64 {

    Registers::Registers() {
        std::fill(gpr_.begin(), gpr_.end(), (u64)0);
        std::fill(xmm_.begin(), xmm_.end(), u128{0, 0});
    }

}