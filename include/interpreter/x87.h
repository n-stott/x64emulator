#ifndef X87_H
#define X87_H

#include "utils/utils.h"
#include <array>

namespace x64 {

    class X87Fpu {
    public:
        X87Fpu() {
            std::fill(stack_.begin(), stack_.end(), f80::fromLongDouble(0));
        }

        void push(f80 val) {
            decrTop();
            stack_[top_] = val;
        }

        f80 pop() {
            f80 val = stack_[top_];
            incrTop();
            return val;
        }

        f80 st(u8 index) {
            return stack_[(top_+index) & 0x7];
        }

    private:

        void incrTop() {
            u8 nextTop = (top_+1);
            C1_ = (nextTop & 0x8);
            top_ = nextTop & 0x7;
        }

        void decrTop() {
            u8 nextTop = (top_-1);
            C1_ = (nextTop & 0x8);
            top_ = nextTop & 0x7;
        }

        std::array<f80, 8> stack_;
        u8 top_ { 0 };
        bool C0_ { false };
        bool C1_ { false };
        bool C2_ { false };
        bool C3_ { false };
    };

}

#endif