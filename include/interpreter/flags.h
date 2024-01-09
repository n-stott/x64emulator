#ifndef FLAGS_H
#define FLAGS_H

#include "types.h"

namespace x64 {

    struct Flags {
        bool carry { false };
        bool zero { false };
        bool sign { false };
        bool overflow { false };
        bool parity { false };
        bool direction { false };

        bool matches(Cond condition) const;

        static bool computeParity(u8 val) {
            bool parity = true;  // parity will be the parity of v
            while (val) {
                parity = !parity;
                val = val & (u8)(val - 1);
            }
            return parity;
        }

        bool sure_ = true;
        bool sureParity_ = true;
        void setUnsure() { sure_ = false; }
        void setSure() { sure_ = true; }
        void setUnsureParity() { sureParity_ = false; }
        void setSureParity() { sureParity_ = true; }
        bool sure() const { return sure_; }
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
            case Cond::NP: return (parity == 0);
            case Cond::NS: return (sign == 0);
            case Cond::NU: return (parity == 0);
            case Cond::O: return (overflow == 1);
            case Cond::P: return (parity == 1);
            case Cond::S: return (sign == 1);
            case Cond::U: return (parity == 1);
        }
        __builtin_unreachable();
    }

}

#endif
