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
        virtual void exec(Sub<R32, R32>) = 0;

        virtual void exec(And<R32, u32>) = 0;
        virtual void exec(And<R32, R32>) = 0;

        virtual void exec(Xor<R32, u32>) = 0;
        virtual void exec(Xor<R32, R32>) = 0;

        virtual void exec(Xchg<R16, R16>) = 0;
        virtual void exec(Xchg<R32, R32>) = 0;

        virtual void exec(Mov<R32, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, u32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(Lea<R32, B>) = 0;
        virtual void exec(Lea<R32, BD>) = 0;
        virtual void exec(Lea<R32, BISD>) = 0;

        virtual void exec(Push<R32>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Push<SignExtended<u8>>) = 0;
        virtual void exec(Pop<R32>) = 0;

        virtual void exec(CallDirect) = 0;
        virtual void exec(CallIndirect<R32>) = 0;
        virtual void exec(Ret) = 0;

        virtual void exec(Leave) = 0;
        virtual void exec(Halt) = 0;
        virtual void exec(Nop) = 0;

        virtual void exec(Shr<R32, u32>) = 0;
        virtual void exec(Shr<R32, Count>) = 0;
        virtual void exec(Sar<R32, u32>) = 0;
        virtual void exec(Sar<R32, Count>) = 0;

        virtual void exec(Test<R32, R32>) = 0;
        virtual void exec(Cmp<R32, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BD>, u8>) = 0;

        virtual void exec(Je) = 0;

    };

}

#endif