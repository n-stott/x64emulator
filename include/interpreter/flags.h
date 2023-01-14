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

    class LazyFlags {
    public:
        enum class Op {
            Add,
            Sub,
        };

        struct CachedOp {
            bool hasValue = false;
            Op op;
            u32 operand0;
            u32 operand1;
        };

        inline bool carry() { if(cache_.hasValue) update(); return carry_; }
        inline bool zero() { if(cache_.hasValue) update(); return zero_; }
        inline bool sign() { if(cache_.hasValue) update(); return sign_; }
        inline bool overflow() { if(cache_.hasValue) update(); return overflow_; }

        bool matches(Cond condition) const;

        void setUnsure() { sure_ = false; }
        void setSure() { sure_ = true; }
        bool sure() const { return sure_; }

        void cacheOperation(Op op, u32 operand0, u32 operand1) {
            cache_.hasValue = true;
            cache_.op = op;
            cache_.operand0 = operand0;
            cache_.operand1 = operand1;
        }

    private:

        void update();

        CachedOp cache_;

        bool carry_ { false };
        bool zero_ { false };
        bool sign_ { false };
        bool overflow_ { false };

        bool sure_ = true;


    };

}

#endif