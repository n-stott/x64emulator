#include "arch/arm64/flags.h"
#include "fmt/format.h"

namespace arm64 {

    std::string Flags::toString() const {
        return fmt::format("{}{}{}{}",
            negative ? "N" : " ",
            zero     ? "Z" : " ",
            carry    ? "C" : " ",
            overflow ? "O" : " "
        );
    }

    static constexpr u8 ZERO_MASK = 0x1;
    static constexpr u8 OVERFLOW_MASK = 0x2;
    static constexpr u8 CARRY_MASK = 0x4;
    static constexpr u8 NEGATIVE_MASK = 0x8;

    u8 Flags::asU8() const {
        u8 res = 0;
        if(zero) res |= ZERO_MASK;
        if(overflow) res |= OVERFLOW_MASK;
        if(carry) res |= CARRY_MASK;
        if(negative) res |= NEGATIVE_MASK;
        return res;
    }

    Flags Flags::fromU8(u8 val) {
        Flags flags;
        flags.zero = (val & ZERO_MASK);
        flags.overflow = (val & OVERFLOW_MASK);
        flags.carry = (val & CARRY_MASK);
        flags.negative = (val & NEGATIVE_MASK);
        return flags;
    }

}