#ifndef INSTRUCTIONNHANDLER_H
#define INSTRUCTIONNHANDLER_H

#include "instructions.h"

namespace x86 {

    class InstructionHandler {
    public:
        ~InstructionHandler() = default;

        virtual void exec(Add<R32, u32>) = 0;
        virtual void exec(Add<R32, R32>) = 0;

        virtual void exec(Sub<R32, SignExtended<u8>>) = 0;

        virtual void exec(Mov<R32, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, u32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(Push<R32>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(CallDirect) = 0;
        virtual void exec(Ret) = 0;

        virtual void exec(Leave) = 0;

        virtual void exec(Test<R32, R32>) = 0;

        virtual void exec(Je) = 0;

    };

}

#endif