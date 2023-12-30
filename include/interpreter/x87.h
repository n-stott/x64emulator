#ifndef X87_H
#define X87_H

#include "utils/utils.h"
#include "types.h"
#include <array>

namespace x64 {

    enum class ROUNDING {
        NEAREST,
        DOWN,
        UP,
        ZERO,
    };

    struct X87Control {
        bool im { 1 };
        bool dm { 1 };
        bool zm { 1 };
        bool om { 1 };
        bool um { 1 };
        bool pm { 1 };

        u8 pc { 0x3 };
        ROUNDING rc { ROUNDING::NEAREST };
        bool x { 0 };

        u16 asWord() const;
        static X87Control fromWord(u16 cw);
    };


    class X87Fpu {
    public:
        X87Fpu();

        void push(f80 val);
        f80 pop();
        f80 st(ST st) const;
        void set(ST st, f80 val);
        u8 top() const { return top_; }


        X87Control& control() { return control_; }

    private:
        void incrTop();
        void decrTop();

        std::array<f80, 8> stack_;
        X87Control control_;
        u8 top_ { 0 };
        // bool C0_ { false }; // unused
        bool C1_ { false };
        // bool C2_ { false }; // unused
        // bool C3_ { false }; // unused
    };

}

#endif