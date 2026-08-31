#ifndef ARM64REGISTERS_H
#define ARM64REGISTERS_H

#include "arch/arm64/types.h"
#include <array>
#include <cassert>
#include <string>

namespace arm64 {

    class Registers {
    private:
        std::array<u64, 34> gpr_;
        std::array<u128, 32> simd_;

    public:
        Registers();

        u64* gprs() { return gpr_.data(); }
        u128* simds() { return simd_.data(); }

        u64 pc() const { return gpr_[(u8)R64::PC]; }
        u64 sp() const { return gpr_[(u8)R64::SP]; }
        u64 lr() const { return gpr_[(u8)R64::X30]; }

        u64& pc() { return gpr_[(u8)R64::PC]; }
        u64& sp() { return gpr_[(u8)R64::SP]; }
        u64& lr() { return gpr_[(u8)R64::X30]; }

        u32 get(R32 reg) const {
            return (u32)gpr_[(u8)reg];
        }

        u64 get(R64 reg) const {
            return gpr_[(u8)reg];
        }
        
        void set(R32 reg, u32 value) {
            assert(reg != R32::ZERO);
            gpr_[(u8)reg] = value;
        }

        void set(R64 reg, u64 value) {
            assert(reg != R64::ZERO);
            gpr_[(u8)reg] = value;
        }

        u64 resolve(Encoding enc) {
            if(enc.increment) {
                assert(!"increment in address not supported yet");
            } else {
                return get(enc.base) + get(enc.index) + enc.offset;
            }
        }

        std::string toString() const;

    };

}

#endif