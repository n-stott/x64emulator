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
        std::array<u64, 3> sys_;

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

        u64 get(D64 reg) const {
            return simd_[(u8)reg].lo;
        }

        u128 get(Q128 reg) const {
            return simd_[(u8)reg];
        }

        u128 get(V128 reg) const {
            return simd_[(u8)reg];
        }
        
        void set(R32 reg, u32 value) {
            assert(reg != R32::ZERO);
            gpr_[(u8)reg] = value;
        }

        void set(R64 reg, u64 value) {
            assert(reg != R64::ZERO);
            gpr_[(u8)reg] = value;
        }

        void set(D64 reg, u64 value) {
            simd_[(u8)reg] = u128{value, (u64)0};
        }

        void set(Q128 reg, u128 value) {
            simd_[(u8)reg] = value;
        }

        void set(V128 reg, u128 value) {
            simd_[(u8)reg] = value;
        }

        u64 get(Sysreg reg) const {
            return sys_[(u8)reg];
        }

        void set(Sysreg reg, u64 value) {
            assert(reg != Sysreg::DCZID_EL0);
            sys_[(u8)reg] = value;
        }

        u64 resolve(Encoding enc) {
            return get(enc.base) + get(enc.index) + enc.offset;
        }

        std::string toString() const;

    };

}

#endif