#ifndef X87_H
#define X87_H

#include "x64/types.h"
#include "utils.h"
#include <array>
#include <string>

namespace x64 {

    enum class FPU_ROUNDING {
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
        FPU_ROUNDING rc { FPU_ROUNDING::NEAREST };
        bool x { 0 };

        u16 asWord() const;
        static X87Control fromWord(u16 cw);
    };

    struct X87Status {
        u8 top { 0 };

        u16 asWord() const;
        static X87Status fromWord(u16 sw);
    };

    struct X87Tag {
        u16 tags { 0xFFFF };

        u16 asWord() const;
        static X87Tag fromWord(u16 tw);
    };

    class X87Fpu {
    public:
        X87Fpu();

        void push(f80 val);
        f80 pop();
        f80 st(ST st) const;
        void set(ST st, f80 val);
        u8 top() const { return status_.top; }


        X87Control& control() { return control_; }
        const X87Control& control() const { return control_; }
        X87Status& status() { return status_; }
        const X87Status& status() const { return status_; }
        X87Tag& tag() { return tag_; }
        const X87Tag& tag() const { return tag_; }

        std::string toString() const;

    private:
        void incrTop();
        void decrTop();

        std::array<f80, 8> stack_;
        X87Control control_;
        X87Status status_;
        X87Tag tag_;
        // bool C0_ { false }; // unused
        bool C1_ { false };
        // bool C2_ { false }; // unused
        // bool C3_ { false }; // unused
    };

}

#endif