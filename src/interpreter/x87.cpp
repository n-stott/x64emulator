#include "interpreter/x87.h"

namespace x64 {

    X87Fpu::X87Fpu() {
        std::fill(stack_.begin(), stack_.end(), f80::fromLongDouble(0));
    }

    void X87Fpu::push(f80 val) {
        decrTop();
        stack_[top_] = val;
    }

    f80 X87Fpu::pop() {
        f80 val = stack_[top_];
        incrTop();
        return val;
    }

    f80 X87Fpu::st(ST st) const {
        return stack_[(top_+(u8)st) & 0x7];
    }

    void X87Fpu::set(ST st, f80 val) {
        stack_[(top_+(u8)st) & 0x7] = val;
    }

    void X87Fpu::incrTop() {
        u8 nextTop = (top_+1);
        C1_ = (nextTop & 0x8);
        top_ = nextTop & 0x7;
    }

    void X87Fpu::decrTop() {
        u8 nextTop = (top_-1);
        C1_ = (nextTop & 0x8);
        top_ = nextTop & 0x7;
    }

    u16 X87Control::asWord() const {
        auto res = (u16)im << 0
                | (u16)dm << 1
                | (u16)zm << 2
                | (u16)om << 3
                | (u16)um << 4
                | (u16)pm << 5
                | (u16)pc << 8
                | (u16)rc << 10
                | (u16)x << 11;
        return (u16)res;
    }

    X87Control X87Control::fromWord(u16 cw) {
        X87Control c;
        c.im = (cw >> 0) & 0x1;
        c.dm = (cw >> 1) & 0x1;
        c.zm = (cw >> 2) & 0x1;
        c.om = (cw >> 3) & 0x1;
        c.um = (cw >> 4) & 0x1;
        c.pm = (cw >> 5) & 0x1;
        c.pc = (cw >> 8) & 0x3;
        c.rc = (ROUNDING)((cw >> 10) & 0x3);
        c.x  = (cw >> 11) & 0x1;
        return c;
    }

}