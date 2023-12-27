#ifndef INSTRUCTIONNHANDLER_H
#define INSTRUCTIONNHANDLER_H

#include "instructions.h"

namespace x64 {

    class InstructionHandler {
    public:
        ~InstructionHandler() = default;

        virtual void exec(const Add<R8, R8>&) = 0;
        virtual void exec(const Add<R8, Imm>&) = 0;
        virtual void exec(const Add<R8, M8>&) = 0;
        virtual void exec(const Add<M8, R8>&) = 0;
        virtual void exec(const Add<M8, Imm>&) = 0;
        virtual void exec(const Add<R16, R16>&) = 0;
        virtual void exec(const Add<R16, Imm>&) = 0;
        virtual void exec(const Add<R16, M16>&) = 0;
        virtual void exec(const Add<M16, R16>&) = 0;
        virtual void exec(const Add<M16, Imm>&) = 0;
        virtual void exec(const Add<R32, R32>&) = 0;
        virtual void exec(const Add<R32, Imm>&) = 0;
        virtual void exec(const Add<R32, M32>&) = 0;
        virtual void exec(const Add<M32, R32>&) = 0;
        virtual void exec(const Add<M32, Imm>&) = 0;
        virtual void exec(const Add<R64, R64>&) = 0;
        virtual void exec(const Add<R64, Imm>&) = 0;
        virtual void exec(const Add<R64, M64>&) = 0;
        virtual void exec(const Add<M64, R64>&) = 0;
        virtual void exec(const Add<M64, Imm>&) = 0;

        virtual void exec(const Adc<R32, R32>&) = 0;
        virtual void exec(const Adc<R32, Imm>&) = 0;
        virtual void exec(const Adc<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Adc<R32, M32>&) = 0;
        virtual void exec(const Adc<M32, R32>&) = 0;
        virtual void exec(const Adc<M32, Imm>&) = 0;

        virtual void exec(const Sub<R8, R8>&) = 0;
        virtual void exec(const Sub<R8, Imm>&) = 0;
        virtual void exec(const Sub<R8, M8>&) = 0;
        virtual void exec(const Sub<M8, R8>&) = 0;
        virtual void exec(const Sub<M8, Imm>&) = 0;
        virtual void exec(const Sub<R16, R16>&) = 0;
        virtual void exec(const Sub<R16, Imm>&) = 0;
        virtual void exec(const Sub<R16, M16>&) = 0;
        virtual void exec(const Sub<M16, R16>&) = 0;
        virtual void exec(const Sub<M16, Imm>&) = 0;
        virtual void exec(const Sub<R32, R32>&) = 0;
        virtual void exec(const Sub<R32, Imm>&) = 0;
        virtual void exec(const Sub<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Sub<R32, M32>&) = 0;
        virtual void exec(const Sub<M32, R32>&) = 0;
        virtual void exec(const Sub<M32, Imm>&) = 0;
        virtual void exec(const Sub<R64, R64>&) = 0;
        virtual void exec(const Sub<R64, Imm>&) = 0;
        virtual void exec(const Sub<R64, SignExtended<u8>>&) = 0;
        virtual void exec(const Sub<R64, M64>&) = 0;
        virtual void exec(const Sub<M64, R64>&) = 0;
        virtual void exec(const Sub<M64, Imm>&) = 0;

        virtual void exec(const Sbb<R32, R32>&) = 0;
        virtual void exec(const Sbb<R32, Imm>&) = 0;
        virtual void exec(const Sbb<R32, SignExtended<u8>>&) = 0;
        virtual void exec(const Sbb<R32, M32>&) = 0;
        virtual void exec(const Sbb<M32, R32>&) = 0;
        virtual void exec(const Sbb<M32, Imm>&) = 0;

        virtual void exec(const Neg<R32>&) = 0;
        virtual void exec(const Neg<M32>&) = 0;
        virtual void exec(const Neg<R64>&) = 0;
        virtual void exec(const Neg<M64>&) = 0;

        virtual void exec(const Mul<R32>&) = 0;
        virtual void exec(const Mul<M32>&) = 0;
        virtual void exec(const Mul<R64>&) = 0;
        virtual void exec(const Mul<M64>&) = 0;

        virtual void exec(const Imul1<R32>&) = 0;
        virtual void exec(const Imul1<M32>&) = 0;
        virtual void exec(const Imul2<R32, R32>&) = 0;
        virtual void exec(const Imul2<R32, M32>&) = 0;
        virtual void exec(const Imul3<R32, R32, Imm>&) = 0;
        virtual void exec(const Imul3<R32, M32, Imm>&) = 0;
        virtual void exec(const Imul1<R64>&) = 0;
        virtual void exec(const Imul1<M64>&) = 0;
        virtual void exec(const Imul2<R64, R64>&) = 0;
        virtual void exec(const Imul2<R64, M64>&) = 0;
        virtual void exec(const Imul3<R64, R64, Imm>&) = 0;
        virtual void exec(const Imul3<R64, M64, Imm>&) = 0;

        virtual void exec(const Div<R32>&) = 0;
        virtual void exec(const Div<M32>&) = 0;
        virtual void exec(const Div<R64>&) = 0;
        virtual void exec(const Div<M64>&) = 0;

        virtual void exec(const Idiv<R32>&) = 0;
        virtual void exec(const Idiv<M32>&) = 0;
        virtual void exec(const Idiv<R64>&) = 0;
        virtual void exec(const Idiv<M64>&) = 0;

        virtual void exec(const And<R8, R8>&) = 0;
        virtual void exec(const And<R8, Imm>&) = 0;
        virtual void exec(const And<R8, M8>&) = 0;
        virtual void exec(const And<R16, Imm>&) = 0;
        virtual void exec(const And<R16, R16>&) = 0;
        virtual void exec(const And<R16, M16>&) = 0;
        virtual void exec(const And<R32, R32>&) = 0;
        virtual void exec(const And<R32, Imm>&) = 0;
        virtual void exec(const And<R32, M32>&) = 0;
        virtual void exec(const And<R64, R64>&) = 0;
        virtual void exec(const And<R64, Imm>&) = 0;
        virtual void exec(const And<R64, M64>&) = 0;
        virtual void exec(const And<M8, R8>&) = 0;
        virtual void exec(const And<M8, Imm>&) = 0;
        virtual void exec(const And<M16, Imm>&) = 0;
        virtual void exec(const And<M16, R16>&) = 0;
        virtual void exec(const And<M32, R32>&) = 0;
        virtual void exec(const And<M32, Imm>&) = 0;
        virtual void exec(const And<M64, R64>&) = 0;
        virtual void exec(const And<M64, Imm>&) = 0;

        virtual void exec(const Or<R8, R8>&) = 0;
        virtual void exec(const Or<R8, Imm>&) = 0;
        virtual void exec(const Or<R8, M8>&) = 0;
        virtual void exec(const Or<M8, R8>&) = 0;
        virtual void exec(const Or<M8, Imm>&) = 0;
        virtual void exec(const Or<R16, M16>&) = 0;
        virtual void exec(const Or<M16, R16>&) = 0;
        virtual void exec(const Or<R32, R32>&) = 0;
        virtual void exec(const Or<R32, Imm>&) = 0;
        virtual void exec(const Or<R32, M32>&) = 0;
        virtual void exec(const Or<M32, R32>&) = 0;
        virtual void exec(const Or<M32, Imm>&) = 0;
        virtual void exec(const Or<R64, R64>&) = 0;
        virtual void exec(const Or<R64, Imm>&) = 0;
        virtual void exec(const Or<R64, M64>&) = 0;
        virtual void exec(const Or<M64, R64>&) = 0;
        virtual void exec(const Or<M64, Imm>&) = 0;

        virtual void exec(const Xor<R8, R8>&) = 0;
        virtual void exec(const Xor<R8, Imm>&) = 0;
        virtual void exec(const Xor<R8, M8>&) = 0;
        virtual void exec(const Xor<M8, Imm>&) = 0;
        virtual void exec(const Xor<R16, Imm>&) = 0;
        virtual void exec(const Xor<R32, Imm>&) = 0;
        virtual void exec(const Xor<R32, R32>&) = 0;
        virtual void exec(const Xor<R32, M32>&) = 0;
        virtual void exec(const Xor<M32, R32>&) = 0;
        virtual void exec(const Xor<R64, Imm>&) = 0;
        virtual void exec(const Xor<R64, R64>&) = 0;
        virtual void exec(const Xor<R64, M64>&) = 0;
        virtual void exec(const Xor<M64, R64>&) = 0;

        virtual void exec(const Not<R32>&) = 0;
        virtual void exec(const Not<M32>&) = 0;
        virtual void exec(const Not<R64>&) = 0;
        virtual void exec(const Not<M64>&) = 0;

        virtual void exec(const Xchg<R16, R16>&) = 0;
        virtual void exec(const Xchg<R32, R32>&) = 0;
        virtual void exec(const Xchg<M32, R32>&) = 0;
        virtual void exec(const Xchg<R64, R64>&) = 0;
        virtual void exec(const Xchg<M64, R64>&) = 0;

        virtual void exec(const Xadd<R16, R16>&) = 0;
        virtual void exec(const Xadd<R32, R32>&) = 0;
        virtual void exec(const Xadd<M32, R32>&) = 0;
        virtual void exec(const Xadd<R64, R64>&) = 0;
        virtual void exec(const Xadd<M64, R64>&) = 0;

        virtual void exec(const Mov<R8, R8>&) = 0;
        virtual void exec(const Mov<R8, Imm>&) = 0;
        virtual void exec(const Mov<R8, M8>&) = 0;
        virtual void exec(const Mov<M8, R8>&) = 0;
        virtual void exec(const Mov<M8, Imm>&) = 0;
        virtual void exec(const Mov<R16, R16>&) = 0;
        virtual void exec(const Mov<R16, Imm>&) = 0;
        virtual void exec(const Mov<R16, M16>&) = 0;
        virtual void exec(const Mov<M16, R16>&) = 0;
        virtual void exec(const Mov<M16, Imm>&) = 0;
        virtual void exec(const Mov<R32, R32>&) = 0;
        virtual void exec(const Mov<R32, Imm>&) = 0;
        virtual void exec(const Mov<R32, M32>&) = 0;
        virtual void exec(const Mov<M32, R32>&) = 0;
        virtual void exec(const Mov<M32, Imm>&) = 0;
        virtual void exec(const Mov<R64, R64>&) = 0;
        virtual void exec(const Mov<R64, Imm>&) = 0;
        virtual void exec(const Mov<R64, M64>&) = 0;
        virtual void exec(const Mov<M64, R64>&) = 0;
        virtual void exec(const Mov<M64, Imm>&) = 0;
        virtual void exec(const Mov<RSSE, RSSE>&) = 0;
        virtual void exec(const Mov<RSSE, MSSE>&) = 0;
        virtual void exec(const Mov<MSSE, RSSE>&) = 0;

        virtual void exec(const Movsx<R32, R8>&) = 0;
        virtual void exec(const Movsx<R32, M8>&) = 0;
        virtual void exec(const Movsx<R64, R8>&) = 0;
        virtual void exec(const Movsx<R64, M8>&) = 0;
        virtual void exec(const Movsx<R32, R16>&) = 0;
        virtual void exec(const Movsx<R32, M16>&) = 0;
        virtual void exec(const Movsx<R64, R16>&) = 0;
        virtual void exec(const Movsx<R64, M16>&) = 0;
        virtual void exec(const Movsx<R32, R32>&) = 0;
        virtual void exec(const Movsx<R32, M32>&) = 0;
        virtual void exec(const Movsx<R64, R32>&) = 0;
        virtual void exec(const Movsx<R64, M32>&) = 0;

        virtual void exec(const Movzx<R16, R8>&) = 0;
        virtual void exec(const Movzx<R32, R8>&) = 0;
        virtual void exec(const Movzx<R32, R16>&) = 0;
        virtual void exec(const Movzx<R32, M8>&) = 0;
        virtual void exec(const Movzx<R32, M16>&) = 0;

        virtual void exec(const Lea<R32, B>&) = 0;
        virtual void exec(const Lea<R32, BD>&) = 0;
        virtual void exec(const Lea<R32, BIS>&) = 0;
        virtual void exec(const Lea<R32, ISD>&) = 0;
        virtual void exec(const Lea<R32, BISD>&) = 0;
        virtual void exec(const Lea<R64, B>&) = 0;
        virtual void exec(const Lea<R64, BD>&) = 0;
        virtual void exec(const Lea<R64, BIS>&) = 0;
        virtual void exec(const Lea<R64, ISD>&) = 0;
        virtual void exec(const Lea<R64, BISD>&) = 0;

        virtual void exec(const Push<SignExtended<u8>>&) = 0;
        virtual void exec(const Push<Imm>&) = 0;
        virtual void exec(const Push<R32>&) = 0;
        virtual void exec(const Push<M32>&) = 0;
        virtual void exec(const Push<R64>&) = 0;
        virtual void exec(const Push<M64>&) = 0;

        virtual void exec(const Pop<R32>&) = 0;
        virtual void exec(const Pop<R64>&) = 0;

        virtual void exec(const CallDirect&) = 0;
        virtual void exec(const CallIndirect<R32>&) = 0;
        virtual void exec(const CallIndirect<M32>&) = 0;
        virtual void exec(const CallIndirect<R64>&) = 0;
        virtual void exec(const CallIndirect<M64>&) = 0;
        virtual void exec(const Ret<>&) = 0;
        virtual void exec(const Ret<Imm>&) = 0;

        virtual void exec(const Leave&) = 0;
        virtual void exec(const Halt&) = 0;
        virtual void exec(const Nop&) = 0;
        virtual void exec(const Ud2&) = 0;
        virtual void exec(const Syscall&) = 0;
        virtual void exec(const NotParsed&) = 0;
        virtual void exec(const Unknown&) = 0;

        virtual void exec(const Cdq&) = 0;
        virtual void exec(const Cqo&) = 0;

        virtual void exec(const Inc<R8>&) = 0;
        virtual void exec(const Inc<M8>&) = 0;
        virtual void exec(const Inc<M16>&) = 0;
        virtual void exec(const Inc<R32>&) = 0;
        virtual void exec(const Inc<M32>&) = 0;

        virtual void exec(const Dec<R8>&) = 0;
        virtual void exec(const Dec<M16>&) = 0;
        virtual void exec(const Dec<R32>&) = 0;
        virtual void exec(const Dec<M32>&) = 0;

        virtual void exec(const Shr<R8, Imm>&) = 0;
        virtual void exec(const Shr<R16, Imm>&) = 0;
        virtual void exec(const Shr<R32, R8>&) = 0;
        virtual void exec(const Shr<R32, Imm>&) = 0;
        virtual void exec(const Shr<R64, R8>&) = 0;
        virtual void exec(const Shr<R64, Imm>&) = 0;

        virtual void exec(const Shl<R32, R8>&) = 0;
        virtual void exec(const Shl<M32, R8>&) = 0;
        virtual void exec(const Shl<R32, Imm>&) = 0;
        virtual void exec(const Shl<M32, Imm>&) = 0;
        virtual void exec(const Shl<R64, R8>&) = 0;
        virtual void exec(const Shl<M64, R8>&) = 0;
        virtual void exec(const Shl<R64, Imm>&) = 0;
        virtual void exec(const Shl<M64, Imm>&) = 0;

        virtual void exec(const Shld<R32, R32, R8>&) = 0;
        virtual void exec(const Shld<R32, R32, Imm>&) = 0;

        virtual void exec(const Shrd<R32, R32, R8>&) = 0;
        virtual void exec(const Shrd<R32, R32, Imm>&) = 0;

        virtual void exec(const Sar<R32, R8>&) = 0;
        virtual void exec(const Sar<R32, Imm>&) = 0;
        virtual void exec(const Sar<M32, Imm>&) = 0;
        virtual void exec(const Sar<R64, R8>&) = 0;
        virtual void exec(const Sar<R64, Imm>&) = 0;
        virtual void exec(const Sar<M64, Imm>&) = 0;

        virtual void exec(const Rol<R32, R8>&) = 0;
        virtual void exec(const Rol<R32, Imm>&) = 0;
        virtual void exec(const Rol<M32, Imm>&) = 0;
        virtual void exec(const Rol<R64, R8>&) = 0;
        virtual void exec(const Rol<R64, Imm>&) = 0;
        virtual void exec(const Rol<M64, Imm>&) = 0;

        virtual void exec(const Ror<R32, R8>&) = 0;
        virtual void exec(const Ror<R32, Imm>&) = 0;
        virtual void exec(const Ror<M32, Imm>&) = 0;
        virtual void exec(const Ror<R64, R8>&) = 0;
        virtual void exec(const Ror<R64, Imm>&) = 0;
        virtual void exec(const Ror<M64, Imm>&) = 0;

        virtual void exec(const Tzcnt<R16, R16>&) = 0;
        virtual void exec(const Tzcnt<R16, M16>&) = 0;
        virtual void exec(const Tzcnt<R32, R32>&) = 0;
        virtual void exec(const Tzcnt<R32, M32>&) = 0;
        virtual void exec(const Tzcnt<R64, R64>&) = 0;
        virtual void exec(const Tzcnt<R64, M64>&) = 0;

        virtual void exec(const Bt<R16, R16>&) = 0;
        virtual void exec(const Bt<R16, Imm>&) = 0;
        virtual void exec(const Bt<R32, R32>&) = 0;
        virtual void exec(const Bt<R32, Imm>&) = 0;
        virtual void exec(const Bt<R64, R64>&) = 0;
        virtual void exec(const Bt<R64, Imm>&) = 0;
        virtual void exec(const Bt<M16, R16>&) = 0;
        virtual void exec(const Bt<M16, Imm>&) = 0;
        virtual void exec(const Bt<M32, R32>&) = 0;
        virtual void exec(const Bt<M32, Imm>&) = 0;
        virtual void exec(const Bt<M64, R64>&) = 0;
        virtual void exec(const Bt<M64, Imm>&) = 0;

        virtual void exec(const Test<R8, R8>&) = 0;
        virtual void exec(const Test<M8, R8>&) = 0;
        virtual void exec(const Test<R8, Imm>&) = 0;
        virtual void exec(const Test<M8, Imm>&) = 0;
        virtual void exec(const Test<R16, R16>&) = 0;
        virtual void exec(const Test<R16, Imm>&) = 0;
        virtual void exec(const Test<M16, R16>&) = 0;
        virtual void exec(const Test<M16, Imm>&) = 0;
        virtual void exec(const Test<R32, R32>&) = 0;
        virtual void exec(const Test<R32, Imm>&) = 0;
        virtual void exec(const Test<M32, R32>&) = 0;
        virtual void exec(const Test<M32, Imm>&) = 0;
        virtual void exec(const Test<R64, R64>&) = 0;
        virtual void exec(const Test<R64, Imm>&) = 0;
        virtual void exec(const Test<M64, R64>&) = 0;
        virtual void exec(const Test<M64, Imm>&) = 0;

        virtual void exec(const Cmp<R8, R8>&) = 0;
        virtual void exec(const Cmp<R8, Imm>&) = 0;
        virtual void exec(const Cmp<R8, M8>&) = 0;
        virtual void exec(const Cmp<M8, R8>&) = 0;
        virtual void exec(const Cmp<M8, Imm>&) = 0;
        virtual void exec(const Cmp<R16, R16>&) = 0;
        virtual void exec(const Cmp<R16, Imm>&) = 0;
        virtual void exec(const Cmp<R16, M16>&) = 0;
        virtual void exec(const Cmp<M16, Imm>&) = 0;
        virtual void exec(const Cmp<M16, R16>&) = 0;
        virtual void exec(const Cmp<R32, R32>&) = 0;
        virtual void exec(const Cmp<R32, Imm>&) = 0;
        virtual void exec(const Cmp<R32, M32>&) = 0;
        virtual void exec(const Cmp<M32, R32>&) = 0;
        virtual void exec(const Cmp<M32, Imm>&) = 0;
        virtual void exec(const Cmp<R64, R64>&) = 0;
        virtual void exec(const Cmp<R64, Imm>&) = 0;
        virtual void exec(const Cmp<R64, M64>&) = 0;
        virtual void exec(const Cmp<M64, R64>&) = 0;
        virtual void exec(const Cmp<M64, Imm>&) = 0;

        virtual void exec(const Cmpxchg<R8, R8>&) = 0;
        virtual void exec(const Cmpxchg<M8, R8>&) = 0;
        virtual void exec(const Cmpxchg<R16, R16>&) = 0;
        virtual void exec(const Cmpxchg<M16, R16>&) = 0;
        virtual void exec(const Cmpxchg<R32, R32>&) = 0;
        virtual void exec(const Cmpxchg<M32, R32>&) = 0;
        virtual void exec(const Cmpxchg<R64, R64>&) = 0;
        virtual void exec(const Cmpxchg<M64, R64>&) = 0;

        virtual void exec(const Set<Cond::AE, R8>&) = 0;
        virtual void exec(const Set<Cond::AE, M8>&) = 0;
        virtual void exec(const Set<Cond::A, R8>&) = 0;
        virtual void exec(const Set<Cond::A, M8>&) = 0;
        virtual void exec(const Set<Cond::B, R8>&) = 0;
        virtual void exec(const Set<Cond::B, M8>&) = 0;
        virtual void exec(const Set<Cond::BE, R8>&) = 0;
        virtual void exec(const Set<Cond::BE, M8>&) = 0;
        virtual void exec(const Set<Cond::E, R8>&) = 0;
        virtual void exec(const Set<Cond::E, M8>&) = 0;
        virtual void exec(const Set<Cond::G, R8>&) = 0;
        virtual void exec(const Set<Cond::G, M8>&) = 0;
        virtual void exec(const Set<Cond::GE, R8>&) = 0;
        virtual void exec(const Set<Cond::GE, M8>&) = 0;
        virtual void exec(const Set<Cond::L, R8>&) = 0;
        virtual void exec(const Set<Cond::L, M8>&) = 0;
        virtual void exec(const Set<Cond::LE, R8>&) = 0;
        virtual void exec(const Set<Cond::LE, M8>&) = 0;
        virtual void exec(const Set<Cond::NE, R8>&) = 0;
        virtual void exec(const Set<Cond::NE, M8>&) = 0;
        virtual void exec(const Set<Cond::NO, R8>&) = 0;
        virtual void exec(const Set<Cond::NO, M8>&) = 0;
        virtual void exec(const Set<Cond::NS, R8>&) = 0;
        virtual void exec(const Set<Cond::NS, M8>&) = 0;
        virtual void exec(const Set<Cond::O, R8>&) = 0;
        virtual void exec(const Set<Cond::O, M8>&) = 0;
        virtual void exec(const Set<Cond::S, R8>&) = 0;
        virtual void exec(const Set<Cond::S, M8>&) = 0;

        virtual void exec(const Jmp<R32>&) = 0;
        virtual void exec(const Jmp<R64>&) = 0;
        virtual void exec(const Jmp<u32>&) = 0;
        virtual void exec(const Jmp<M32>&) = 0;
        virtual void exec(const Jmp<M64>&) = 0;
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
        virtual void exec(const Jcc<Cond::O>&) = 0;
        virtual void exec(const Jcc<Cond::NO>&) = 0;
        virtual void exec(const Jcc<Cond::P>&) = 0;
        virtual void exec(const Jcc<Cond::NP>&) = 0;

        virtual void exec(const Bsr<R32, R32>&) = 0;
        virtual void exec(const Bsr<R64, R64>&) = 0;

        virtual void exec(const Bsf<R32, R32>&) = 0;
        virtual void exec(const Bsf<R64, R64>&) = 0;

        virtual void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&) = 0;
        virtual void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&) = 0;
        virtual void exec(const Rep<Movs<M64, M64>>&) = 0;
        
        virtual void exec(const Rep<Stos<M32, R32>>&) = 0;
        virtual void exec(const Rep<Stos<M64, R64>>&) = 0;

        virtual void exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>&) = 0;

        virtual void exec(const Cmov<Cond::AE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::AE, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::A, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::A, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::B, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::B, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::E, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::E, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::G, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::G, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::L, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::L, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::S, R32, R32>&) = 0;
        virtual void exec(const Cmov<Cond::S, R32, M32>&) = 0;
        virtual void exec(const Cmov<Cond::AE, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::AE, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::A, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::A, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::BE, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::B, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::B, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::E, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::E, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::GE, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::G, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::G, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::LE, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::L, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::L, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::NE, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::NS, R64, M64>&) = 0;
        virtual void exec(const Cmov<Cond::S, R64, R64>&) = 0;
        virtual void exec(const Cmov<Cond::S, R64, M64>&) = 0;

        virtual void exec(const Cwde&) = 0;
        virtual void exec(const Cdqe&) = 0;

        virtual void exec(const Pxor<RSSE, RSSE>&) = 0;
        virtual void exec(const Pxor<RSSE, MSSE>&) = 0;

        virtual void exec(const Movaps<RSSE, RSSE>&) = 0;
        virtual void exec(const Movaps<MSSE, RSSE>&) = 0;
        virtual void exec(const Movaps<RSSE, MSSE>&) = 0;
        virtual void exec(const Movaps<MSSE, MSSE>&) = 0;

        virtual void exec(const Movd<RSSE, R32>&) = 0;
        virtual void exec(const Movd<R32, RSSE>&) = 0;

        virtual void exec(const Movq<RSSE, R64>&) = 0;
        virtual void exec(const Movq<R64, RSSE>&) = 0;
        virtual void exec(const Movq<RSSE, M64>&) = 0;
        virtual void exec(const Movq<M64, RSSE>&) = 0;

        virtual void exec(const Movss<RSSE, M32>&) = 0;
        virtual void exec(const Movss<M32, RSSE>&) = 0;

        virtual void exec(const Movsd<RSSE, M64>&) = 0;
        virtual void exec(const Movsd<M64, RSSE>&) = 0;

        virtual void exec(const Addss<RSSE, RSSE>&) = 0;
        virtual void exec(const Addss<RSSE, M32>&) = 0;
        virtual void exec(const Addsd<RSSE, RSSE>&) = 0;
        virtual void exec(const Addsd<RSSE, M64>&) = 0;

        virtual void exec(const Subss<RSSE, RSSE>&) = 0;
        virtual void exec(const Subss<RSSE, M32>&) = 0;
        virtual void exec(const Subsd<RSSE, RSSE>&) = 0;
        virtual void exec(const Subsd<RSSE, M64>&) = 0;

        virtual void exec(const Mulsd<RSSE, RSSE>&) = 0;
        virtual void exec(const Mulsd<RSSE, M64>&) = 0;

        virtual void exec(const Comiss<RSSE, RSSE>&) = 0;
        virtual void exec(const Comiss<RSSE, M32>&) = 0;
        virtual void exec(const Comisd<RSSE, RSSE>&) = 0;
        virtual void exec(const Comisd<RSSE, M64>&) = 0;
        virtual void exec(const Ucomiss<RSSE, RSSE>&) = 0;
        virtual void exec(const Ucomiss<RSSE, M32>&) = 0;
        virtual void exec(const Ucomisd<RSSE, RSSE>&) = 0;
        virtual void exec(const Ucomisd<RSSE, M64>&) = 0;

        virtual void exec(const Cvtsi2sd<RSSE, R32>&) = 0;
        virtual void exec(const Cvtsi2sd<RSSE, M32>&) = 0;
        virtual void exec(const Cvtsi2sd<RSSE, R64>&) = 0;
        virtual void exec(const Cvtsi2sd<RSSE, M64>&) = 0;
        virtual void exec(const Cvtss2sd<RSSE, RSSE>&) = 0;
        virtual void exec(const Cvtss2sd<RSSE, M32>&) = 0;

        virtual void exec(const Por<RSSE, RSSE>&) = 0;
        virtual void exec(const Xorpd<RSSE, RSSE>&) = 0;
        virtual void exec(const Movhps<RSSE, M64>&) = 0;

        virtual void exec(const Punpcklbw<RSSE, RSSE>&) = 0;
        virtual void exec(const Punpcklwd<RSSE, RSSE>&) = 0;
        virtual void exec(const Punpcklqdq<RSSE, RSSE>&) = 0;

        virtual void exec(const Pshufd<RSSE, RSSE, Imm>&) = 0;
        virtual void exec(const Pshufd<RSSE, MSSE, Imm>&) = 0;

        virtual void exec(const Pcmpeqb<RSSE, RSSE>&) = 0;
        virtual void exec(const Pcmpeqb<RSSE, MSSE>&) = 0;

        virtual void exec(const Pmovmskb<R32, RSSE>&) = 0;

        virtual void exec(const Pminub<RSSE, RSSE>&) = 0;
        virtual void exec(const Pminub<RSSE, MSSE>&) = 0;

        virtual void exec(const Ptest<RSSE, RSSE>&) = 0;
        virtual void exec(const Ptest<RSSE, MSSE>&) = 0;

        virtual void exec(const Rdtsc&) = 0;

        virtual void exec(const Cpuid&) = 0;
        virtual void exec(const Xgetbv&) = 0;

        virtual void exec(const Rdpkru&) = 0;
        virtual void exec(const Wrpkru&) = 0;

        virtual void resolveFunctionName(const CallDirect&) const = 0;
    };

}

#endif
