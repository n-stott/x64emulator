#ifndef FLAGS_H
#define FLAGS_H

#include "types.h"

namespace x86 {

    struct Flags {
        bool carry { false };
        bool zero { false };
        bool sign { false };
        bool overflow { false };

        bool matches(Cond condition) const;

        bool sure_ = true;
        void setUnsure() { sure_ = false; }
        void setSure() { sure_ = true; }
        bool sure() const { return sure_; }
    };

}

#endif
