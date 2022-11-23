#ifndef INSTRUCTIONNHANDLER_H
#define INSTRUCTIONNHANDLER_H

#include "instructions.h"

namespace x86 {

    class InstructionHandler {
    public:
        ~InstructionHandler() = default;

        virtual void exec(Add<R32, R32>) = 0;
        virtual void exec(Add<R32, Imm<u32>>) = 0;
        virtual void exec(Add<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Add<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Add<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Add<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Add<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(Sub<R32, R32>) = 0;
        virtual void exec(Sub<R32, Imm<u32>>) = 0;
        virtual void exec(Sub<R32, SignExtended<u8>>) = 0;
        virtual void exec(Sub<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Sub<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Sub<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Sub<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Sub<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(Sbb<R32, R32>) = 0;
        virtual void exec(Sbb<R32, Imm<u32>>) = 0;
        virtual void exec(Sbb<R32, SignExtended<u8>>) = 0;
        virtual void exec(Sbb<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Sbb<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Sbb<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Sbb<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Sbb<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(And<R32, Imm<u32>>) = 0;
        virtual void exec(And<R32, R32>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(And<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Or<R32, Imm<u32>>) = 0;
        virtual void exec(Or<R32, R32>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Xor<R32, Imm<u32>>) = 0;
        virtual void exec(Xor<R32, R32>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Xchg<R16, R16>) = 0;
        virtual void exec(Xchg<R32, R32>) = 0;

        virtual void exec(Mov<R8, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BISD>>) = 0;
        virtual void exec(Mov<R32, R32>) = 0;
        virtual void exec(Mov<R32, Imm<u32>>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Mov<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, B>, R8>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BD>, R8>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BIS>, R8>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BIS>, Imm<u8>>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BISD>, R8>) = 0;
        virtual void exec(Mov<Addr<Size::BYTE, BISD>, Imm<u8>>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Mov<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(Lea<R32, B>) = 0;
        virtual void exec(Lea<R32, BD>) = 0;
        virtual void exec(Lea<R32, BIS>) = 0;
        virtual void exec(Lea<R32, ISD>) = 0;
        virtual void exec(Lea<R32, BISD>) = 0;

        virtual void exec(Push<R32>) = 0;
        virtual void exec(Push<SignExtended<u8>>) = 0;
        virtual void exec(Push<Imm<u32>>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, ISD>>) = 0;
        virtual void exec(Push<Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Pop<R32>) = 0;

        virtual void exec(CallDirect) = 0;
        virtual void exec(CallIndirect<R32>) = 0;
        virtual void exec(CallIndirect<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Ret) = 0;

        virtual void exec(Leave) = 0;
        virtual void exec(Halt) = 0;
        virtual void exec(Nop) = 0;
        virtual void exec(Ud2) = 0;

        virtual void exec(Inc<R8>) = 0;
        virtual void exec(Inc<R32>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BISD>>) = 0;

        virtual void exec(Dec<R8>) = 0;
        virtual void exec(Dec<R32>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BISD>>) = 0;

        virtual void exec(Shr<R32, Imm<u32>>) = 0;
        virtual void exec(Shr<R32, Count>) = 0;
        virtual void exec(Shl<R32, Imm<u32>>) = 0;
        virtual void exec(Shl<R32, Count>) = 0;
        virtual void exec(Sar<R32, Imm<u32>>) = 0;
        virtual void exec(Sar<R32, Count>) = 0;

        virtual void exec(Test<R8, R8>) = 0;
        virtual void exec(Test<R32, R32>) = 0;

        virtual void exec(Cmp<R8, R8>) = 0;
        virtual void exec(Cmp<R8, Imm<u8>>) = 0;
        virtual void exec(Cmp<R16, R16>) = 0;
        virtual void exec(Cmp<R32, R32>) = 0;
        virtual void exec(Cmp<R32, Imm<u32>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(Jmp) = 0;
        virtual void exec(Jne) = 0;
        virtual void exec(Je) = 0;
        virtual void exec(Jae) = 0;
        virtual void exec(Jbe) = 0;
        virtual void exec(Jge) = 0;
        virtual void exec(Jle) = 0;
        virtual void exec(Ja) = 0;
        virtual void exec(Jb) = 0;
        virtual void exec(Jg) = 0;
        virtual void exec(Jl) = 0;

    };

}

#endif