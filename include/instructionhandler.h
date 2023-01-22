#ifndef INSTRUCTIONNHANDLER_H
#define INSTRUCTIONNHANDLER_H

#include "instructions.h"

namespace x86 {

    class InstructionHandler {
    public:
        ~InstructionHandler() = default;

        virtual void exec(const Add<R32, R32>&) = 0;
        virtual void exec(const Add<R32, Imm<u32>>&) = 0;
        virtual void exec(const Add<R32, M32>&) = 0;
        virtual void exec(const Add<M32, R32>&) = 0;
        virtual void exec(const Add<M32, Imm<u32>>&) = 0;

        virtual void exec(const Adc<R32, R32>&) = 0;
        virtual void exec(const Adc<R32, Imm<u32>>&) = 0;
        virtual void exec(const Adc<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Adc<R32, M32>&) = 0;
        virtual void exec(const Adc<M32, R32>&) = 0;
        virtual void exec(const Adc<M32, Imm<u32>>&) = 0;

        virtual void exec(const Sub<R32, R32>&) = 0;
        virtual void exec(const Sub<R32, Imm<u32>>&) = 0;
        virtual void exec(const Sub<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Sub<R32, M32>&) = 0;
        virtual void exec(const Sub<M32, R32>&) = 0;
        virtual void exec(const Sub<M32, Imm<u32>>&) = 0;

        virtual void exec(const Sbb<R32, R32>&) = 0;
        virtual void exec(const Sbb<R32, Imm<u32>>&) = 0;
        virtual void exec(const Sbb<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Sbb<R32, M32>&) = 0;
        virtual void exec(const Sbb<M32, R32>&) = 0;
        virtual void exec(const Sbb<M32, Imm<u32>>&) = 0;

        virtual void exec(const Neg<R32>&) = 0;
        virtual void exec(const Neg<M32>&) = 0;

        virtual void exec(const Mul<R32>&) = 0;
        virtual void exec(const Mul<M32>&) = 0;

        virtual void exec(const Imul1<R32>&) = 0;
        virtual void exec(const Imul1<M32>&) = 0;
        virtual void exec(const Imul2<R32, R32>&) = 0;
        virtual void exec(const Imul2<R32, M32>&) = 0;
        virtual void exec(const Imul3<R32, R32, Imm<u32>>&) = 0;
        virtual void exec(const Imul3<R32, M32, Imm<u32>>&) = 0;

        virtual void exec(const Div<R32>&) = 0;
        virtual void exec(const Div<M32>&) = 0;

        virtual void exec(const Idiv<R32>&) = 0;
        virtual void exec(const Idiv<M32>&) = 0;

        virtual void exec(const And<R8, R8>&) = 0;
        virtual void exec(const And<R8, Imm<u8>>&) = 0;
        virtual void exec(const And<R8, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const And<R8, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const And<R16, Addr<Size::WORD, B>>&) = 0;
        virtual void exec(const And<R16, Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const And<R32, R32>&) = 0;
        virtual void exec(const And<R32, Imm<u32>>&) = 0;
        virtual void exec(const And<R32, M32>&) = 0;
        virtual void exec(const And<Addr<Size::BYTE, B>, R8>&) = 0;
        virtual void exec(const And<Addr<Size::BYTE, B>, Imm<u8>>&) = 0;
        virtual void exec(const And<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const And<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const And<Addr<Size::BYTE, BIS>, Imm<u8>>&) = 0;
        virtual void exec(const And<Addr<Size::WORD, B>, R16>&) = 0;
        virtual void exec(const And<Addr<Size::WORD, BD>, R16>&) = 0;
        virtual void exec(const And<M32, R32>&) = 0;
        virtual void exec(const And<M32, Imm<u32>>&) = 0;

        virtual void exec(const Or<R8, R8>&) = 0;
        virtual void exec(const Or<R8, Imm<u8>>&) = 0;
        virtual void exec(const Or<R8, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Or<R8, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Or<R16, Addr<Size::WORD, B>>&) = 0;
        virtual void exec(const Or<R16, Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const Or<R32, R32>&) = 0;
        virtual void exec(const Or<R32, Imm<u32>>&) = 0;
        virtual void exec(const Or<R32, M32>&) = 0;
        virtual void exec(const Or<Addr<Size::BYTE, B>, R8>&) = 0;
        virtual void exec(const Or<Addr<Size::BYTE, B>, Imm<u8>>&) = 0;
        virtual void exec(const Or<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const Or<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const Or<Addr<Size::WORD, B>, R16>&) = 0;
        virtual void exec(const Or<Addr<Size::WORD, BD>, R16>&) = 0;
        virtual void exec(const Or<M32, R32>&) = 0;
        virtual void exec(const Or<M32, Imm<u32>>&) = 0;

        virtual void exec(const Xor<R8, Imm<u8>>&) = 0;
        virtual void exec(const Xor<R8, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Xor<R16, Imm<u16>>&) = 0;
        virtual void exec(const Xor<R32, Imm<u32>>&) = 0;
        virtual void exec(const Xor<R32, R32>&) = 0;
        virtual void exec(const Xor<R32, M32>&) = 0;
        virtual void exec(const Xor<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const Xor<M32, R32>&) = 0;

        virtual void exec(const Not<R32>&) = 0;
        virtual void exec(const Not<M32>&) = 0;

        virtual void exec(const Xchg<R16, R16>&) = 0;
        virtual void exec(const Xchg<R32, R32>&) = 0;

        virtual void exec(const Mov<R8, R8>&) = 0;
        virtual void exec(const Mov<R8, Imm<u8>>&) = 0;
        virtual void exec(const Mov<R8, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Mov<R8, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Mov<R8, Addr<Size::BYTE, BIS>>&) = 0;
        virtual void exec(const Mov<R8, Addr<Size::BYTE, BISD>>&) = 0;
        virtual void exec(const Mov<R16, R16>&) = 0;
        virtual void exec(const Mov<R16, Imm<u16>>&) = 0;
        virtual void exec(const Mov<R16, Addr<Size::WORD, B>>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, B>, R16>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, B>, Imm<u16>>&) = 0;
        virtual void exec(const Mov<R16, Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BD>, R16>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BD>, Imm<u16>>&) = 0;
        virtual void exec(const Mov<R16, Addr<Size::WORD, BIS>>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BIS>, R16>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BIS>, Imm<u16>>&) = 0;
        virtual void exec(const Mov<R16, Addr<Size::WORD, BISD>>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BISD>, R16>&) = 0;
        virtual void exec(const Mov<Addr<Size::WORD, BISD>, Imm<u16>>&) = 0;
        virtual void exec(const Mov<R32, R32>&) = 0;
        virtual void exec(const Mov<R32, Imm<u32>>&) = 0;
        virtual void exec(const Mov<R32, M32>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, B>, R8>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, B>, Imm<u8>>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BIS>, R8>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BIS>, Imm<u8>>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BISD>, R8>&) = 0;
        virtual void exec(const Mov<Addr<Size::BYTE, BISD>, Imm<u8>>&) = 0;
        virtual void exec(const Mov<M32, R32>&) = 0;
        virtual void exec(const Mov<M32, Imm<u32>>&) = 0;

        virtual void exec(const Movsx<R32, R8>&) = 0;
        virtual void exec(const Movsx<R32, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Movsx<R32, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Movsx<R32, Addr<Size::BYTE, BIS>>&) = 0;
        virtual void exec(const Movsx<R32, Addr<Size::BYTE, BISD>>&) = 0;

        virtual void exec(const Movzx<R16, R8>&) = 0;
        virtual void exec(const Movzx<R32, R8>&) = 0;
        virtual void exec(const Movzx<R32, R16>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::BYTE, BIS>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::BYTE, BISD>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::WORD, B>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::WORD, BIS>>&) = 0;
        virtual void exec(const Movzx<R32, Addr<Size::WORD, BISD>>&) = 0;

        virtual void exec(const Lea<R32, B>&) = 0;
        virtual void exec(const Lea<R32, BD>&) = 0;
        virtual void exec(const Lea<R32, BIS>&) = 0;
        virtual void exec(const Lea<R32, ISD>&) = 0;
        virtual void exec(const Lea<R32, BISD>&) = 0;

        virtual void exec(const Push<R32>&) = 0;
        virtual void exec(const Push<SignExtended<u8>>&) = 0;
        virtual void exec(const Push<Imm<u32>>&) = 0;
        virtual void exec(const Push<M32>&) = 0;
        virtual void exec(const Pop<R32>&) = 0;

        virtual void exec(const CallDirect&) = 0;
        virtual void exec(const CallIndirect<R32>&) = 0;
        virtual void exec(const CallIndirect<M32>&) = 0;
        virtual void exec(const Ret<>&) = 0;
        virtual void exec(const Ret<Imm<u16>>&) = 0;

        virtual void exec(const Leave&) = 0;
        virtual void exec(const Halt&) = 0;
        virtual void exec(const Nop&) = 0;
        virtual void exec(const Ud2&) = 0;
        virtual void exec(const Cdq&) = 0;
        virtual void exec(const NotParsed&) = 0;
        virtual void exec(const Unknown&) = 0;

        virtual void exec(const Inc<R8>&) = 0;
        virtual void exec(const Inc<R32>&) = 0;
        virtual void exec(const Inc<Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Inc<Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Inc<Addr<Size::BYTE, BIS>>&) = 0;
        virtual void exec(const Inc<Addr<Size::BYTE, BISD>>&) = 0;
        virtual void exec(const Inc<Addr<Size::WORD, B>>&) = 0;
        virtual void exec(const Inc<Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const Inc<Addr<Size::WORD, BIS>>&) = 0;
        virtual void exec(const Inc<Addr<Size::WORD, BISD>>&) = 0;
        virtual void exec(const Inc<M32>&) = 0;

        virtual void exec(const Dec<R8>&) = 0;
        virtual void exec(const Dec<R32>&) = 0;
        virtual void exec(const Dec<Addr<Size::WORD, BD>>&) = 0;
        virtual void exec(const Dec<M32>&) = 0;

        virtual void exec(const Shr<R8, Imm<u8>>&) = 0;
        virtual void exec(const Shr<R8, Count>&) = 0;
        virtual void exec(const Shr<R16, Count>&) = 0;
        virtual void exec(const Shr<R16, Imm<u8>>&) = 0;
        virtual void exec(const Shr<R32, R8>&) = 0;
        virtual void exec(const Shr<R32, Imm<u32>>&) = 0;
        virtual void exec(const Shr<R32, Count>&) = 0;

        virtual void exec(const Shl<R32, R8>&) = 0;
        virtual void exec(const Shl<R32, Imm<u32>>&) = 0;
        virtual void exec(const Shl<R32, Count>&) = 0;
        virtual void exec(const Shl<M32, Imm<u32>>&) = 0;

        virtual void exec(const Shld<R32, R32, R8>&) = 0;
        virtual void exec(const Shld<R32, R32, Imm<u8>>&) = 0;

        virtual void exec(const Shrd<R32, R32, R8>&) = 0;
        virtual void exec(const Shrd<R32, R32, Imm<u8>>&) = 0;

        virtual void exec(const Sar<R32, R8>&) = 0;
        virtual void exec(const Sar<R32, Imm<u32>>&) = 0;
        virtual void exec(const Sar<R32, Count>&) = 0;
        virtual void exec(const Sar<M32, Count>&) = 0;

        virtual void exec(const Rol<R32, R8>&) = 0;
        virtual void exec(const Rol<R32, Imm<u8>>&) = 0;
        virtual void exec(const Rol<M32, Imm<u8>>&) = 0;

        virtual void exec(const Test<R8, R8>&) = 0;
        virtual void exec(const Test<R8, Imm<u8>>&) = 0;
        virtual void exec(const Test<R16, R16>&) = 0;
        virtual void exec(const Test<R32, R32>&) = 0;
        virtual void exec(const Test<R32, Imm<u32>>&) = 0;
        virtual void exec(const Test<Addr<Size::BYTE, B>, Imm<u8>>&) = 0;
        virtual void exec(const Test<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const Test<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const Test<Addr<Size::BYTE, BIS>, Imm<u8>>&) = 0;
        virtual void exec(const Test<Addr<Size::BYTE, BISD>, Imm<u8>>&) = 0;
        virtual void exec(const Test<M32, R32>&) = 0;
        virtual void exec(const Test<M32, Imm<u32>>&) = 0;

        virtual void exec(const Cmp<R8, R8>&) = 0;
        virtual void exec(const Cmp<R8, Imm<u8>>&) = 0;
        virtual void exec(const Cmp<R8, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Cmp<R8, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Cmp<R8, Addr<Size::BYTE, BIS>>&) = 0;
        virtual void exec(const Cmp<R8, Addr<Size::BYTE, BISD>>&) = 0;
        virtual void exec(const Cmp<R16, R16>&) = 0;
        virtual void exec(const Cmp<R16, Imm<u16>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::WORD, B>, Imm<u16>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::WORD, BD>, Imm<u16>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::WORD, BIS>, R16>&) = 0;
        virtual void exec(const Cmp<R32, R32>&) = 0;
        virtual void exec(const Cmp<R32, Imm<u32>>&) = 0;
        virtual void exec(const Cmp<R32, M32>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, B>, R8>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, B>, Imm<u8>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BD>, Imm<u8>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BIS>, R8>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BISD>, R8>&) = 0;
        virtual void exec(const Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>&) = 0;
        virtual void exec(const Cmp<M32, R32>&) = 0;
        virtual void exec(const Cmp<M32, Imm<u32>>&) = 0;

        virtual void exec(const Cmpxchg<R8, R8>&) = 0;
        virtual void exec(const Cmpxchg<R16, R16>&) = 0;
        virtual void exec(const Cmpxchg<Addr<Size::WORD, BIS>, R16>&) = 0;
        virtual void exec(const Cmpxchg<R32, R32>&) = 0;
        virtual void exec(const Cmpxchg<R32, Imm<u32>>&) = 0;
        virtual void exec(const Cmpxchg<Addr<Size::BYTE, B>, R8>&) = 0;
        virtual void exec(const Cmpxchg<Addr<Size::BYTE, BD>, R8>&) = 0;
        virtual void exec(const Cmpxchg<Addr<Size::BYTE, BIS>, R8>&) = 0;
        virtual void exec(const Cmpxchg<Addr<Size::BYTE, BISD>, R8>&) = 0;
        virtual void exec(const Cmpxchg<M32, R32>&) = 0;

        virtual void exec(const Set<Cond::AE, R8>&) = 0;
        virtual void exec(const Set<Cond::AE, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::AE, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::A, R8>&) = 0;
        virtual void exec(const Set<Cond::A, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::A, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::B, R8>&) = 0;
        virtual void exec(const Set<Cond::B, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::B, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::BE, R8>&) = 0;
        virtual void exec(const Set<Cond::BE, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::BE, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::E, R8>&) = 0;
        virtual void exec(const Set<Cond::E, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::E, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::G, R8>&) = 0;
        virtual void exec(const Set<Cond::G, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::G, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::GE, R8>&) = 0;
        virtual void exec(const Set<Cond::GE, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::GE, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::L, R8>&) = 0;
        virtual void exec(const Set<Cond::L, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::L, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::LE, R8>&) = 0;
        virtual void exec(const Set<Cond::LE, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::LE, Addr<Size::BYTE, BD>>&) = 0;
        virtual void exec(const Set<Cond::NE, R8>&) = 0;
        virtual void exec(const Set<Cond::NE, Addr<Size::BYTE, B>>&) = 0;
        virtual void exec(const Set<Cond::NE, Addr<Size::BYTE, BD>>&) = 0;

        virtual void exec(const Jmp<R32>&) = 0;
        virtual void exec(const Jmp<u32>&) = 0;
        virtual void exec(const Jmp<M32>&) = 0;
        virtual void exec(const Jcc<Cond::NE>&) = 0;
        virtual void exec(const Jcc<Cond::E>&) = 0;
        virtual void exec(const Jcc<Cond::AE>&) = 0;
        virtual void exec(const Jcc<Cond::BE>&) = 0;
        virtual void exec(const Jcc<Cond::GE>&) = 0;
        virtual void exec(const Jcc<Cond::LE>&) = 0;
        virtual void exec(const Jcc<Cond::A>&) = 0;
        virtual void exec(const Jcc<Cond::B>&) = 0;
        virtual void exec(const Jcc<Cond::G>&) = 0;
        virtual void exec(const Jcc<Cond::L>&) = 0;
        virtual void exec(const Jcc<Cond::S>&) = 0;
        virtual void exec(const Jcc<Cond::NS>&) = 0;

        virtual void exec(const Bsr<R32, R32>&) = 0;
        virtual void exec(const Bsf<R32, R32>&) = 0;
        virtual void exec(const Bsf<R32, M32>&) = 0;

        virtual void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&) = 0;
        virtual void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&) = 0;
        
        virtual void exec(const Rep<Stos<Addr<Size::DWORD, B>, R32>>&) = 0;

        virtual void exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>&) = 0;

        virtual void exec(const Cmov<Cond::AE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::A, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::A, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::B, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::B, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::E, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::E, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::G, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::G, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::L, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::L, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R32, Addr<Size::DWORD, BD>>&) = 0;
        virtual void exec(const Cmov<Cond::S, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::S, R32, Addr<Size::DWORD, BD>>&) = 0;

        virtual void exec(const Cwde&) = 0;

    };

}

#endif