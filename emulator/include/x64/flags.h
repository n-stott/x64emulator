#ifndef FLAGS_H
#define FLAGS_H

#include "x64/types.h"
#include <optional>
#include <string>

namespace x64 {

    struct Flags {
        bool carry { false };
        bool zero { false };
        bool sign { false };
        bool overflow { false };
        bool direction { false };

        bool matches(Cond condition) const;

        void setParity(bool value) {
            parity_ = value;
            awaitingParity_.reset();
        }
        void deferParity(u8 value) {
            awaitingParity_ = value;
        }
        bool parity() const {
            if(!!awaitingParity_) {
                parity_ = computeParity(awaitingParity_.value());
                awaitingParity_.reset();
            }
            return parity_;
        }

        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 PARITY_MASK = 0x4;
        static constexpr u64 ZERO_MASK = 0x40;
        static constexpr u64 SIGN_MASK = 0x80;
        static constexpr u64 OVERFLOW_MASK = 0x800;

        static Flags fromRflags(u64 rflags) {
            Flags flags;
            flags.carry = rflags & CARRY_MASK;
            flags.setParity(rflags & PARITY_MASK);
            flags.zero = rflags & ZERO_MASK;
            flags.sign = rflags & SIGN_MASK;
            flags.overflow = rflags & OVERFLOW_MASK;
            return flags;
        }

        u64 toRflags() const {
            u64 rflags = 0;
            rflags |= (carry ? CARRY_MASK : 0);
            rflags |= (parity() ? PARITY_MASK : 0);
            rflags |= (zero ? ZERO_MASK : 0);
            rflags |= (sign ? SIGN_MASK : 0);
            rflags |= (overflow ? OVERFLOW_MASK : 0);
            return rflags;
        }

        std::string toString() const;

    private:
        mutable std::optional<u8> awaitingParity_;
        mutable bool parity_ { false };

        static bool computeParity(u8 val) {
            bool parity = true;  // parity will be the parity of v
            while (val) {
                parity = !parity;
                val = val & (u8)(val - 1);
            }
            return parity;
        }
    };



    inline bool Flags::matches(Cond condition) const {
        switch(condition) {
            case Cond::A: return (carry == 0 && zero == 0);
            case Cond::AE: return (carry == 0);
            case Cond::B: return (carry == 1);
            case Cond::BE: return (carry == 1 || zero == 1);
            case Cond::E: return (zero == 1);
            case Cond::G: return (zero == 0 && sign == overflow);
            case Cond::GE: return (sign == overflow);
            case Cond::L: return (sign != overflow);
            case Cond::LE: return (zero == 1 || sign != overflow);
            case Cond::NB: return (carry == 0);
            case Cond::NBE: return (carry == 0 && zero == 0);
            case Cond::NE: return (zero == 0);
            case Cond::NO: return (overflow == 0);
            case Cond::NP: return (parity() == 0);
            case Cond::NS: return (sign == 0);
            case Cond::NU: return (parity() == 0);
            case Cond::O: return (overflow == 1);
            case Cond::P: return (parity() == 1);
            case Cond::S: return (sign == 1);
            case Cond::U: return (parity() == 1);
        }
        __builtin_unreachable();
    }

}

#endif
