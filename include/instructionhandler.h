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

        virtual void exec(Adc<R32, R32>) = 0;
        virtual void exec(Adc<R32, Imm<u32>>) = 0;
        virtual void exec(Adc<R32, SignExtended<u8>>) = 0;
        virtual void exec(Adc<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Adc<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Adc<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Adc<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Adc<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

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

        virtual void exec(Neg<R32>) = 0;
        virtual void exec(Neg<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Neg<Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(Mul<R32>) = 0;
        virtual void exec(Mul<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Mul<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Mul<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Mul<Addr<Size::DWORD, BISD>>) = 0;

        virtual void exec(Imul1<R32>) = 0;
        virtual void exec(Imul1<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Imul2<R32, R32>) = 0;
        virtual void exec(Imul2<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Imul2<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Imul2<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Imul2<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Imul3<R32, R32, Imm<u32>>) = 0;
        virtual void exec(Imul3<R32, Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Imul3<R32, Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Imul3<R32, Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;

        virtual void exec(Div<R32>) = 0;
        virtual void exec(Div<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Div<Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(Idiv<R32>) = 0;
        virtual void exec(Idiv<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Idiv<Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(And<R8, R8>) = 0;
        virtual void exec(And<R8, Imm<u8>>) = 0;
        virtual void exec(And<R8, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(And<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(And<R16, Addr<Size::WORD, B>>) = 0;
        virtual void exec(And<R16, Addr<Size::WORD, BD>>) = 0;
        virtual void exec(And<R32, R32>) = 0;
        virtual void exec(And<R32, Imm<u32>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(And<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(And<Addr<Size::BYTE, B>, R8>) = 0;
        virtual void exec(And<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(And<Addr<Size::BYTE, BD>, R8>) = 0;
        virtual void exec(And<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(And<Addr<Size::BYTE, BIS>, Imm<u8>>) = 0;
        virtual void exec(And<Addr<Size::WORD, B>, R16>) = 0;
        virtual void exec(And<Addr<Size::WORD, BD>, R16>) = 0;
        virtual void exec(And<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(And<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Or<R8, R8>) = 0;
        virtual void exec(Or<R8, Imm<u8>>) = 0;
        virtual void exec(Or<R8, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Or<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Or<R16, Addr<Size::WORD, B>>) = 0;
        virtual void exec(Or<R16, Addr<Size::WORD, BD>>) = 0;
        virtual void exec(Or<R32, R32>) = 0;
        virtual void exec(Or<R32, Imm<u32>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Or<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Or<Addr<Size::BYTE, B>, R8>) = 0;
        virtual void exec(Or<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(Or<Addr<Size::BYTE, BD>, R8>) = 0;
        virtual void exec(Or<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Or<Addr<Size::WORD, B>, R16>) = 0;
        virtual void exec(Or<Addr<Size::WORD, BD>, R16>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Or<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Xor<R8, Imm<u8>>) = 0;
        virtual void exec(Xor<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Xor<R16, Imm<u16>>) = 0;
        virtual void exec(Xor<R32, Imm<u32>>) = 0;
        virtual void exec(Xor<R32, R32>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Xor<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Xor<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Xor<Addr<Size::DWORD, BISD>, R32>) = 0;

        virtual void exec(Not<R32>) = 0;
        virtual void exec(Not<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Not<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Not<Addr<Size::DWORD, BIS>>) = 0;

        virtual void exec(Xchg<R16, R16>) = 0;
        virtual void exec(Xchg<R32, R32>) = 0;

        virtual void exec(Mov<R8, R8>) = 0;
        virtual void exec(Mov<R8, Imm<u8>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Mov<R8, Addr<Size::BYTE, BISD>>) = 0;
        virtual void exec(Mov<R16, R16>) = 0;
        virtual void exec(Mov<R16, Imm<u16>>) = 0;
        virtual void exec(Mov<R16, Addr<Size::WORD, B>>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, B>, R16>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, B>, Imm<u16>>) = 0;
        virtual void exec(Mov<R16, Addr<Size::WORD, BD>>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BD>, R16>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BD>, Imm<u16>>) = 0;
        virtual void exec(Mov<R16, Addr<Size::WORD, BIS>>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BIS>, R16>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BIS>, Imm<u16>>) = 0;
        virtual void exec(Mov<R16, Addr<Size::WORD, BISD>>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BISD>, R16>) = 0;
        virtual void exec(Mov<Addr<Size::WORD, BISD>, Imm<u16>>) = 0;
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

        virtual void exec(Movsx<R32, R8>) = 0;
        virtual void exec(Movsx<R32, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Movsx<R32, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Movsx<R32, Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Movsx<R32, Addr<Size::BYTE, BISD>>) = 0;

        virtual void exec(Movzx<R16, R8>) = 0;
        virtual void exec(Movzx<R32, R8>) = 0;
        virtual void exec(Movzx<R32, R16>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::BYTE, BISD>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::WORD, B>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::WORD, BD>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::WORD, BIS>>) = 0;
        virtual void exec(Movzx<R32, Addr<Size::WORD, BISD>>) = 0;

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
        virtual void exec(CallIndirect<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(CallIndirect<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(CallIndirect<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Ret<>) = 0;
        virtual void exec(Ret<Imm<u16>>) = 0;

        virtual void exec(Leave) = 0;
        virtual void exec(Halt) = 0;
        virtual void exec(Nop) = 0;
        virtual void exec(Ud2) = 0;
        virtual void exec(Cdq) = 0;
        virtual void exec(NotParsed) = 0;
        virtual void exec(Unknown) = 0;

        virtual void exec(Inc<R8>) = 0;
        virtual void exec(Inc<R32>) = 0;
        virtual void exec(Inc<Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Inc<Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Inc<Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Inc<Addr<Size::BYTE, BISD>>) = 0;
        virtual void exec(Inc<Addr<Size::WORD, B>>) = 0;
        virtual void exec(Inc<Addr<Size::WORD, BD>>) = 0;
        virtual void exec(Inc<Addr<Size::WORD, BIS>>) = 0;
        virtual void exec(Inc<Addr<Size::WORD, BISD>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Inc<Addr<Size::DWORD, BISD>>) = 0;

        virtual void exec(Dec<R8>) = 0;
        virtual void exec(Dec<R32>) = 0;
        virtual void exec(Dec<Addr<Size::WORD, BD>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Dec<Addr<Size::DWORD, BISD>>) = 0;

        virtual void exec(Shr<R8, Imm<u8>>) = 0;
        virtual void exec(Shr<R8, Count>) = 0;
        virtual void exec(Shr<R16, Count>) = 0;
        virtual void exec(Shr<R32, R8>) = 0;
        virtual void exec(Shr<R32, Imm<u32>>) = 0;
        virtual void exec(Shr<R32, Count>) = 0;

        virtual void exec(Shl<R32, R8>) = 0;
        virtual void exec(Shl<R32, Imm<u32>>) = 0;
        virtual void exec(Shl<R32, Count>) = 0;
        virtual void exec(Shl<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;

        virtual void exec(Shld<R32, R32, R8>) = 0;
        virtual void exec(Shld<R32, R32, Imm<u8>>) = 0;

        virtual void exec(Shrd<R32, R32, R8>) = 0;
        virtual void exec(Shrd<R32, R32, Imm<u8>>) = 0;

        virtual void exec(Sar<R32, R8>) = 0;
        virtual void exec(Sar<R32, Imm<u32>>) = 0;
        virtual void exec(Sar<R32, Count>) = 0;
        virtual void exec(Sar<Addr<Size::DWORD, B>, Count>) = 0;
        virtual void exec(Sar<Addr<Size::DWORD, BD>, Count>) = 0;

        virtual void exec(Rol<R32, R8>) = 0;
        virtual void exec(Rol<R32, Imm<u8>>) = 0;
        virtual void exec(Rol<Addr<Size::DWORD, BD>, Imm<u8>>) = 0;

        virtual void exec(Test<R8, R8>) = 0;
        virtual void exec(Test<R8, Imm<u8>>) = 0;
        virtual void exec(Test<R16, R16>) = 0;
        virtual void exec(Test<R32, R32>) = 0;
        virtual void exec(Test<R32, Imm<u32>>) = 0;
        virtual void exec(Test<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(Test<Addr<Size::BYTE, BD>, R8>) = 0;
        virtual void exec(Test<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Test<Addr<Size::BYTE, BIS>, Imm<u8>>) = 0;
        virtual void exec(Test<Addr<Size::BYTE, BISD>, Imm<u8>>) = 0;
        virtual void exec(Test<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Test<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Test<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;

        virtual void exec(Cmp<R8, R8>) = 0;
        virtual void exec(Cmp<R8, Imm<u8>>) = 0;
        virtual void exec(Cmp<R8, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Cmp<R8, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Cmp<R8, Addr<Size::BYTE, BIS>>) = 0;
        virtual void exec(Cmp<R8, Addr<Size::BYTE, BISD>>) = 0;
        virtual void exec(Cmp<R16, R16>) = 0;
        virtual void exec(Cmp<Addr<Size::WORD, B>, Imm<u16>>) = 0;
        virtual void exec(Cmp<Addr<Size::WORD, BD>, Imm<u16>>) = 0;
        virtual void exec(Cmp<Addr<Size::WORD, BIS>, R16>) = 0;
        virtual void exec(Cmp<R32, R32>) = 0;
        virtual void exec(Cmp<R32, Imm<u32>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BIS>>) = 0;
        virtual void exec(Cmp<R32, Addr<Size::DWORD, BISD>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, B>, R8>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, B>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BD>, R8>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BD>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BIS>, R8>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BISD>, R8>) = 0;
        virtual void exec(Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, B>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, B>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BD>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BD>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BIS>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BIS>, Imm<u32>>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BISD>, R32>) = 0;
        virtual void exec(Cmp<Addr<Size::DWORD, BISD>, Imm<u32>>) = 0;

        virtual void exec(Set<Cond::AE, R8>) = 0;
        virtual void exec(Set<Cond::AE, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::AE, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::A, R8>) = 0;
        virtual void exec(Set<Cond::A, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::A, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::B, R8>) = 0;
        virtual void exec(Set<Cond::B, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::B, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::BE, R8>) = 0;
        virtual void exec(Set<Cond::BE, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::BE, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::E, R8>) = 0;
        virtual void exec(Set<Cond::E, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::E, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::G, R8>) = 0;
        virtual void exec(Set<Cond::G, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::G, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::GE, R8>) = 0;
        virtual void exec(Set<Cond::GE, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::GE, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::L, R8>) = 0;
        virtual void exec(Set<Cond::L, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::L, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::LE, R8>) = 0;
        virtual void exec(Set<Cond::LE, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::LE, Addr<Size::BYTE, BD>>) = 0;
        virtual void exec(Set<Cond::NE, R8>) = 0;
        virtual void exec(Set<Cond::NE, Addr<Size::BYTE, B>>) = 0;
        virtual void exec(Set<Cond::NE, Addr<Size::BYTE, BD>>) = 0;

        virtual void exec(Jmp<R32>) = 0;
        virtual void exec(Jmp<u32>) = 0;
        virtual void exec(Jmp<Addr<Size::DWORD, B>>) = 0;
        virtual void exec(Jmp<Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Jcc<Cond::NE>) = 0;
        virtual void exec(Jcc<Cond::E>) = 0;
        virtual void exec(Jcc<Cond::AE>) = 0;
        virtual void exec(Jcc<Cond::BE>) = 0;
        virtual void exec(Jcc<Cond::GE>) = 0;
        virtual void exec(Jcc<Cond::LE>) = 0;
        virtual void exec(Jcc<Cond::A>) = 0;
        virtual void exec(Jcc<Cond::B>) = 0;
        virtual void exec(Jcc<Cond::G>) = 0;
        virtual void exec(Jcc<Cond::L>) = 0;
        virtual void exec(Jcc<Cond::S>) = 0;
        virtual void exec(Jcc<Cond::NS>) = 0;

        virtual void exec(Bsr<R32, R32>) = 0;
        virtual void exec(Bsf<R32, R32>) = 0;
        virtual void exec(Bsf<R32, Addr<Size::DWORD, BD>>) = 0;

        virtual void exec(Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>) = 0;
        virtual void exec(Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>) = 0;
        
        virtual void exec(Rep<Stos<Addr<Size::DWORD, B>, R32>>) = 0;

        virtual void exec(RepNZ<Scas<R8, Addr<Size::BYTE, B>>>) = 0;

        virtual void exec(Cmov<Cond::AE, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::A, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::A, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::BE, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::B, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::B, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::E, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::E, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::GE, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::G, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::G, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::LE, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::L, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::L, R32, Addr<Size::DWORD, BD>>) = 0;
        virtual void exec(Cmov<Cond::NE, R32, R32>) = 0;
        virtual void exec(Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>>) = 0;

    };

}

#endif