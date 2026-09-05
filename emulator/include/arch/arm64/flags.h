#ifndef ARM64FLAGS_H
#define ARM64FLAGS_H

#include "arch/arm64/types.h"
#include <optional>
#include <string>

namespace arm64 {

    struct Flags {
        bool negative { false };
        bool zero { false };
        bool carry { false };
        bool overflow { false };

        bool matches(Cond condition) const;

        u8 asU8() const;
        static Flags fromU8(u8);

        std::string toString() const;

    private:
    };

    inline bool Flags::matches(Cond condition) const {
        switch(condition) {
            case Cond::EQ: return zero == 1;
            case Cond::NE: return zero == 0;
            case Cond::CS: return carry == 1;
            case Cond::CC: return carry == 0;
            case Cond::MI: return negative == 1;
            case Cond::PL: return negative == 0;
            case Cond::VS: return overflow == 1;
            case Cond::VC: return overflow == 0;
            case Cond::HI: return carry == 1 && zero == 0;
            case Cond::LS: return !(carry == 1 && zero == 0);
            case Cond::GE: return negative == overflow;
            case Cond::LT: return negative != overflow;
            case Cond::GT: return zero == 0 && negative == overflow;
            case Cond::LE: return !(zero == 0 && negative == overflow);
            case Cond::AL: return true;
        }
        UNREACHABLE();
    }

}

#endif
