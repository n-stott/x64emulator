#ifndef FLAGS_H
#define FLAGS_H

#include "types.h"

namespace x86 {

    struct Flags {
        bool carry;
        bool zero;
        bool sign;
        bool overflow;

        bool matches(Cond condition) const;

        bool sure_ = true;
        void setUnsure() { sure_ = false; }
        void setSure() { sure_ = true; }
        bool sure() const { return sure_; }
    };

}

#endif