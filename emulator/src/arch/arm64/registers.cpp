#include "arch/arm64/registers.h"
#include <fmt/format.h>

namespace arm64 {

    Registers::Registers() {
        std::fill(gpr_.begin(), gpr_.end(), (u64)0);
        std::fill(simd_.begin(), simd_.end(), u128{0, 0});
        std::fill(sys_.begin(), sys_.end(), (u64)0);

        sys_[(u8)Sysreg::DCZID_EL0] = 0x4;
    }

    std::string Registers::toString() const {
        std::string res;
        for(size_t i = 0; i < 31; ++i) {
            if(i > 0 && i%4 == 0) res += '\n';
            res += fmt::format("X{:<2}={:0>12x}  ", i, gpr_[i]);
        }
        res += '\n';
        res += fmt::format("PC ={:0>12x}  SP ={:0>12x}\n", get(R64::PC), get(R64::SP));
        return res;
    }

}