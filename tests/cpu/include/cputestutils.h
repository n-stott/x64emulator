#ifndef CPUTESTUTILS_H
#define CPUTESTUTILS_H

#include "x64/flags.h"

[[maybe_unused]] static x64::Flags fromRflags(u64 rflags) {
    static constexpr u64 CARRY_MASK = 0x1;
    static constexpr u64 PARITY_MASK = 0x4;
    static constexpr u64 ZERO_MASK = 0x40;
    static constexpr u64 SIGN_MASK = 0x80;
    static constexpr u64 OVERFLOW_MASK = 0x800;
    x64::Flags flags;
    flags.carry = rflags & CARRY_MASK;
    flags.setParity(rflags & PARITY_MASK);
    flags.zero = rflags & ZERO_MASK;
    flags.sign = rflags & SIGN_MASK;
    flags.overflow = rflags & OVERFLOW_MASK;
    return flags;
}

#endif