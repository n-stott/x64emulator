#ifndef CPU_H
#define CPU_H

#include "interpreter/registers.h"
#include "interpreter/flags.h"
#include "interpreter/x87.h"
#include "instructions/allinstructions.h"

namespace x64 {

    class VM;
    class Mmu;
    struct X86Instruction;

    class Cpu {
    public:
        Cpu(VM* vm, Mmu* mmu) : vm_(vm), mmu_(mmu) { }

    private:
        friend class VM;
        
        VM* vm_;
        Mmu* mmu_;
        Flags flags_;
        Registers regs_;
        X87Fpu x87fpu_;

        struct FPUState {
            u16 fcw;
            u16 fsw;
            u32 unused0;
            u64 unused1;
            u128 fpu1;
            u128 st0;
            u128 st1;
            u128 st2;
            u128 st3;
            u128 st4;
            u128 st5;
            u128 st6;
            u128 st7;
            u128 xmm0;
            u128 xmm1;
            u128 xmm2;
            u128 xmm3;
            u128 xmm4;
            u128 xmm5;
            u128 xmm6;
            u128 xmm7;
            u128 xmm8;
            u128 xmm9;
            u128 xmm10;
            u128 xmm11;
            u128 xmm12;
            u128 xmm13;
            u128 xmm14;
            u128 xmm15;
            u128 reserved0;
            u128 reserved1;
            u128 reserved2;
            u128 available0;
            u128 available1;
            u128 available2;
        };

        static_assert(sizeof(FPUState) == 512, "FPUState must be 512 bytes");

        FPUState getFpuState() const;
        void setFpuState(const FPUState&);

        ROUNDING roundingMode() const;

        u8  get(R8 reg) const  { return regs_.get(reg); }
        u16 get(R16 reg) const { return regs_.get(reg); }
        u32 get(R32 reg) const { return regs_.get(reg); }
        u64 get(R64 reg) const { return regs_.get(reg); }
        Xmm get(RSSE reg) const { return regs_.get(reg); }

        template<typename T>
        T  get(Imm immediate) const;

        u8  get(Ptr8 ptr) const;
        u16 get(Ptr16 ptr) const;
        u32 get(Ptr32 ptr) const;
        u64 get(Ptr64 ptr) const;
        f80 get(Ptr80 ptr) const;
        Xmm get(Ptr128 ptr) const;

        u64 resolve(B addr) const { return regs_.resolve(addr); }
        u64 resolve(BD addr) const { return regs_.resolve(addr); }
        u64 resolve(BIS addr) const { return regs_.resolve(addr); }
        u64 resolve(ISD addr) const { return regs_.resolve(addr); }
        u64 resolve(BISD addr) const { return regs_.resolve(addr); }
        u64 resolve(SO addr) const { return regs_.resolve(addr); }

        template<Size size, typename Enc>
        SPtr<size> resolve(Addr<size, Enc> addr) const { return regs_.resolve(addr); }

        Ptr8 resolve(const M8& m8) const { return regs_.resolve(m8); }
        Ptr16 resolve(const M16& m16) const { return regs_.resolve(m16); }
        Ptr32 resolve(const M32& m32) const { return regs_.resolve(m32); }
        Ptr64 resolve(const M64& m64) const { return regs_.resolve(m64); }
        Ptr80 resolve(const M80& m80) const { return regs_.resolve(m80); }
        Ptr128 resolve(const MSSE& msse) const { return regs_.resolve(msse); }
        Ptr224 resolve(const M224& m224) const { return regs_.resolve(m224); }

        void set(R8 reg, u8 value) { regs_.set(reg, value); }
        void set(R16 reg, u16 value) { regs_.set(reg, value); }
        void set(R32 reg, u32 value) { regs_.set(reg, value); }
        void set(R64 reg, u64 value) { regs_.set(reg, value); }
        void set(RSSE reg, Xmm value) { regs_.set(reg, value); }

        void set(Ptr8 ptr, u8 value);
        void set(Ptr16 ptr, u16 value);
        void set(Ptr32 ptr, u32 value);
        void set(Ptr64 ptr, u64 value);
        void set(Ptr80 ptr, f80 value);
        void set(Ptr128 ptr, Xmm value);

        u8 get(RM8 reg) const;
        u16 get(RM16 reg) const;
        u32 get(RM32 reg) const;
        u64 get(RM64 reg) const;
        Xmm get(RMSSE reg) const;

        void set(RM8 reg, u8 value);
        void set(RM16 reg, u16 value);
        void set(RM32 reg, u32 value);
        void set(RM64 reg, u64 value);
        void set(RMSSE reg, Xmm value);

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        void push64(u64 value);
        u8 pop8();
        u16 pop16();
        u32 pop32();
        u64 pop64();

        template<typename Dst>
        void execSet(Cond cond, Dst dst);

        template<typename Dst>
        void execCmpxchg32Impl(Dst dst, u32 src);

        template<typename Dst>
        void execCmpxchg64Impl(Dst dst, u64 src);

    public:
        void exec(const Add<RM8, RM8>&);
        void exec(const Add<RM8, Imm>&);
        void exec(const Add<RM16, RM16>&);
        void exec(const Add<RM16, Imm>&);
        void exec(const Add<RM32, RM32>&);
        void exec(const Add<RM32, Imm>&);
        void exec(const Add<RM64, RM64>&);
        void exec(const Add<RM64, Imm>&);

        void exec(const Adc<RM8, RM8>&);
        void exec(const Adc<RM8, Imm>&);
        void exec(const Adc<RM16, RM16>&);
        void exec(const Adc<RM16, Imm>&);
        void exec(const Adc<RM32, RM32>&);
        void exec(const Adc<RM32, Imm>&);
        void exec(const Adc<RM64, RM64>&);
        void exec(const Adc<RM64, Imm>&);

        void exec(const Sub<RM8, RM8>&);
        void exec(const Sub<RM8, Imm>&);
        void exec(const Sub<RM16, RM16>&);
        void exec(const Sub<RM16, Imm>&);
        void exec(const Sub<RM32, RM32>&);
        void exec(const Sub<RM32, Imm>&);
        void exec(const Sub<RM64, RM64>&);
        void exec(const Sub<RM64, Imm>&);

        void exec(const Sbb<RM8, RM8>&);
        void exec(const Sbb<RM8, Imm>&);
        void exec(const Sbb<RM16, RM16>&);
        void exec(const Sbb<RM16, Imm>&);
        void exec(const Sbb<RM32, RM32>&);
        void exec(const Sbb<RM32, Imm>&);
        void exec(const Sbb<RM64, RM64>&);
        void exec(const Sbb<RM64, Imm>&);

        void exec(const Neg<RM8>&);
        void exec(const Neg<RM16>&);
        void exec(const Neg<RM32>&);
        void exec(const Neg<RM64>&);

        void exec(const Mul<RM32>&);
        void exec(const Mul<RM64>&);

        void exec(const Imul1<RM32>&);
        void exec(const Imul2<R32, RM32>&);
        void exec(const Imul3<R32, RM32, Imm>&);
        void exec(const Imul1<RM64>&);
        void exec(const Imul2<R64, RM64>&);
        void exec(const Imul3<R64, RM64, Imm>&);

        void exec(const Div<RM32>&);
        void exec(const Div<RM64>&);

        void exec(const Idiv<RM32>&);
        void exec(const Idiv<RM64>&);

        void exec(const And<RM8, RM8>&);
        void exec(const And<RM8, Imm>&);
        void exec(const And<RM16, RM16>&);
        void exec(const And<RM16, Imm>&);
        void exec(const And<RM32, RM32>&);
        void exec(const And<RM32, Imm>&);
        void exec(const And<RM64, RM64>&);
        void exec(const And<RM64, Imm>&);

        void exec(const Or<RM8, RM8>&);
        void exec(const Or<RM8, Imm>&);
        void exec(const Or<RM16, RM16>&);
        void exec(const Or<RM16, Imm>&);
        void exec(const Or<RM32, RM32>&);
        void exec(const Or<RM32, Imm>&);
        void exec(const Or<RM64, RM64>&);
        void exec(const Or<RM64, Imm>&);

        void exec(const Xor<RM8, RM8>&);
        void exec(const Xor<RM8, Imm>&);
        void exec(const Xor<RM16, RM16>&);
        void exec(const Xor<RM16, Imm>&);
        void exec(const Xor<RM32, RM32>&);
        void exec(const Xor<RM32, Imm>&);
        void exec(const Xor<RM64, RM64>&);
        void exec(const Xor<RM64, Imm>&);

        void exec(const Not<RM8>&);
        void exec(const Not<RM16>&);
        void exec(const Not<RM32>&);
        void exec(const Not<RM64>&);

        void exec(const Xchg<RM16, R16>&);
        void exec(const Xchg<RM32, R32>&);
        void exec(const Xchg<RM64, R64>&);

        void exec(const Xadd<RM16, R16>&);
        void exec(const Xadd<RM32, R32>&);
        void exec(const Xadd<RM64, R64>&);

        void exec(const Mov<RM8, RM8>&);
        void exec(const Mov<RM8, Imm>&);
        void exec(const Mov<RM16, RM16>&);
        void exec(const Mov<RM16, Imm>&);
        void exec(const Mov<RM32, RM32>&);
        void exec(const Mov<RM32, Imm>&);
        void exec(const Mov<RM64, RM64>&);
        void exec(const Mov<RM64, Imm>&);
        void exec(const Mov<RMSSE, RMSSE>&);

        void exec(const Movsx<R16, RM8>&);
        void exec(const Movsx<R32, RM8>&);
        void exec(const Movsx<R32, RM16>&);
        void exec(const Movsx<R64, RM8>&);
        void exec(const Movsx<R64, RM16>&);
        void exec(const Movsx<R64, RM32>&);

        void exec(const Movzx<R16, RM8>&);
        void exec(const Movzx<R32, RM8>&);
        void exec(const Movzx<R32, RM16>&);
        void exec(const Movzx<R64, RM8>&);
        void exec(const Movzx<R64, RM16>&);
        void exec(const Movzx<R64, RM32>&);

        void exec(const Lea<R32, B>&);
        void exec(const Lea<R32, BD>&);
        void exec(const Lea<R32, BIS>&);
        void exec(const Lea<R32, ISD>&);
        void exec(const Lea<R32, BISD>&);
        void exec(const Lea<R64, B>&);
        void exec(const Lea<R64, BD>&);
        void exec(const Lea<R64, BIS>&);
        void exec(const Lea<R64, ISD>&);
        void exec(const Lea<R64, BISD>&);

        void exec(const Push<Imm>&);
        void exec(const Push<RM32>&);
        void exec(const Push<RM64>&);

        void exec(const Pop<R32>&);
        void exec(const Pop<R64>&);

        void exec(const CallDirect&);
        void exec(const CallIndirect<RM32>&);
        void exec(const CallIndirect<RM64>&);
        void exec(const Ret<>&);
        void exec(const Ret<Imm>&);

        void exec(const Leave&);
        void exec(const Halt&);
        void exec(const Nop&);
        void exec(const Ud2&);
        void exec(const Syscall&);
        void exec(const NotParsed&);
        void exec(const Unknown&);

        void exec(const Cdq&);
        void exec(const Cqo&);

        void exec(const Inc<RM8>&);
        void exec(const Inc<RM16>&);
        void exec(const Inc<RM32>&);
        void exec(const Inc<RM64>&);

        void exec(const Dec<RM8>&);
        void exec(const Dec<RM16>&);
        void exec(const Dec<RM32>&);
        void exec(const Dec<RM64>&);

        void exec(const Shr<RM8, R8>&);
        void exec(const Shr<RM8, Imm>&);
        void exec(const Shr<RM16, R8>&);
        void exec(const Shr<RM16, Imm>&);
        void exec(const Shr<RM32, R8>&);
        void exec(const Shr<RM32, Imm>&);
        void exec(const Shr<RM64, R8>&);
        void exec(const Shr<RM64, Imm>&);

        void exec(const Shl<RM8, R8>&);
        void exec(const Shl<RM8, Imm>&);
        void exec(const Shl<RM16, R8>&);
        void exec(const Shl<RM16, Imm>&);
        void exec(const Shl<RM32, R8>&);
        void exec(const Shl<RM32, Imm>&);
        void exec(const Shl<RM64, R8>&);
        void exec(const Shl<RM64, Imm>&);

        void exec(const Shld<RM32, R32, R8>&);
        void exec(const Shld<RM32, R32, Imm>&);
        void exec(const Shld<RM64, R64, R8>&);
        void exec(const Shld<RM64, R64, Imm>&);

        void exec(const Shrd<RM32, R32, R8>&);
        void exec(const Shrd<RM32, R32, Imm>&);
        void exec(const Shrd<RM64, R64, R8>&);
        void exec(const Shrd<RM64, R64, Imm>&);

        void exec(const Sar<RM8, R8>&);
        void exec(const Sar<RM8, Imm>&);
        void exec(const Sar<RM16, R8>&);
        void exec(const Sar<RM16, Imm>&);
        void exec(const Sar<RM32, R8>&);
        void exec(const Sar<RM32, Imm>&);
        void exec(const Sar<RM64, R8>&);
        void exec(const Sar<RM64, Imm>&);

        void exec(const Rol<RM8, R8>&);
        void exec(const Rol<RM8, Imm>&);
        void exec(const Rol<RM16, R8>&);
        void exec(const Rol<RM16, Imm>&);
        void exec(const Rol<RM32, R8>&);
        void exec(const Rol<RM32, Imm>&);
        void exec(const Rol<RM64, R8>&);
        void exec(const Rol<RM64, Imm>&);

        void exec(const Ror<RM8, R8>&);
        void exec(const Ror<RM8, Imm>&);
        void exec(const Ror<RM16, R8>&);
        void exec(const Ror<RM16, Imm>&);
        void exec(const Ror<RM32, R8>&);
        void exec(const Ror<RM32, Imm>&);
        void exec(const Ror<RM64, R8>&);
        void exec(const Ror<RM64, Imm>&);

        void exec(const Tzcnt<R16, RM16>&);
        void exec(const Tzcnt<R32, RM32>&);
        void exec(const Tzcnt<R64, RM64>&);

        void exec(const Bt<RM16, R16>&);
        void exec(const Bt<RM16, Imm>&);
        void exec(const Bt<RM32, R32>&);
        void exec(const Bt<RM32, Imm>&);
        void exec(const Bt<RM64, R64>&);
        void exec(const Bt<RM64, Imm>&);

        void exec(const Btr<RM16, R16>&);
        void exec(const Btr<RM16, Imm>&);
        void exec(const Btr<RM32, R32>&);
        void exec(const Btr<RM32, Imm>&);
        void exec(const Btr<RM64, R64>&);
        void exec(const Btr<RM64, Imm>&);

        void exec(const Btc<RM16, R16>&);
        void exec(const Btc<RM16, Imm>&);
        void exec(const Btc<RM32, R32>&);
        void exec(const Btc<RM32, Imm>&);
        void exec(const Btc<RM64, R64>&);
        void exec(const Btc<RM64, Imm>&);

        void exec(const Bts<RM16, R16>&);
        void exec(const Bts<RM16, Imm>&);
        void exec(const Bts<RM32, R32>&);
        void exec(const Bts<RM32, Imm>&);
        void exec(const Bts<RM64, R64>&);
        void exec(const Bts<RM64, Imm>&);

        void exec(const Test<RM8, R8>&);
        void exec(const Test<RM8, Imm>&);
        void exec(const Test<RM16, R16>&);
        void exec(const Test<RM16, Imm>&);
        void exec(const Test<RM32, R32>&);
        void exec(const Test<RM32, Imm>&);
        void exec(const Test<RM64, R64>&);
        void exec(const Test<RM64, Imm>&);

        void exec(const Cmp<RM8, RM8>&);
        void exec(const Cmp<RM8, Imm>&);
        void exec(const Cmp<RM16, RM16>&);
        void exec(const Cmp<RM16, Imm>&);
        void exec(const Cmp<RM32, RM32>&);
        void exec(const Cmp<RM32, Imm>&);
        void exec(const Cmp<RM64, RM64>&);
        void exec(const Cmp<RM64, Imm>&);

        void exec(const Cmpxchg<RM8, R8>&);
        void exec(const Cmpxchg<RM16, R16>&);
        void exec(const Cmpxchg<RM32, R32>&);
        void exec(const Cmpxchg<RM64, R64>&);

        void exec(const Set<RM8>&);

        void exec(const Jmp<RM32>&);
        void exec(const Jmp<RM64>&);
        void exec(const Jmp<u32>&);
        void exec(const Jcc&);

        void exec(const Bsr<R32, R32>&);
        void exec(const Bsr<R64, R64>&);

        void exec(const Bsf<R32, R32>&);
        void exec(const Bsf<R64, R64>&);

        void exec(const Cld&);
        void exec(const Std&);

        void exec(const Movs<M64, M64>&);
        void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&);
        void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&);
        void exec(const Rep<Movs<M32, M32>>&);
        void exec(const Rep<Movs<M64, M64>>&);
        
        void exec(const Rep<Cmps<M8, M8>>&);
        
        void exec(const Rep<Stos<M32, R32>>&);
        void exec(const Rep<Stos<M64, R64>>&);

        void exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>&);

        void exec(const Cmov<R32, RM32>&);
        void exec(const Cmov<R64, RM64>&);

        void exec(const Cwde&);
        void exec(const Cdqe&);

        void exec(const Bswap<R32>&);
        void exec(const Bswap<R64>&);

        void exec(const Pxor<RSSE, RMSSE>&);

        void exec(const Movaps<RMSSE, RMSSE>&);

        void exec(const Movd<RSSE, RM32>&);
        void exec(const Movd<R32, RSSE>&);
        void exec(const Movd<RSSE, RM64>&);
        void exec(const Movd<R64, RSSE>&);

        void exec(const Movq<RSSE, RM64>&);
        void exec(const Movq<RM64, RSSE>&);

        void exec(const Fldz&);
        void exec(const Fld1&);
        void exec(const Fld<ST>&);
        void exec(const Fld<M32>&);
        void exec(const Fld<M64>&);
        void exec(const Fld<M80>&);
        void exec(const Fild<M16>&);
        void exec(const Fild<M32>&);
        void exec(const Fild<M64>&);
        void exec(const Fstp<ST>&);
        void exec(const Fstp<M32>&);
        void exec(const Fstp<M64>&);
        void exec(const Fstp<M80>&);
        void exec(const Fistp<M16>&);
        void exec(const Fistp<M32>&);
        void exec(const Fistp<M64>&);
        void exec(const Fxch<ST>&);

        void exec(const Faddp<ST>&);
        void exec(const Fsubrp<ST>&);
        void exec(const Fmul1<M32>&);
        void exec(const Fmul1<M64>&);
        void exec(const Fdiv<ST, ST>&);
        void exec(const Fdivp<ST, ST>&);

        void exec(const Fcomi<ST>&);
        void exec(const Fucomi<ST>&);
        void exec(const Frndint&);

        void exec(const Fcmov<ST>&);

        void exec(const Fnstcw<M16>&);
        void exec(const Fldcw<M16>&);

        void exec(const Fnstsw<R16>&);
        void exec(const Fnstsw<M16>&);

        void exec(const Fnstenv<M224>&);
        void exec(const Fldenv<M224>&);

        void exec(const Movss<RSSE, M32>&);
        void exec(const Movss<M32, RSSE>&);

        void exec(const Movsd<RSSE, M64>&);
        void exec(const Movsd<M64, RSSE>&);
        
        void exec(const Addss<RSSE, RSSE>&);
        void exec(const Addss<RSSE, M32>&);
        void exec(const Addsd<RSSE, RSSE>&);
        void exec(const Addsd<RSSE, M64>&);

        void exec(const Subss<RSSE, RSSE>&);
        void exec(const Subss<RSSE, M32>&);
        void exec(const Subsd<RSSE, RSSE>&);
        void exec(const Subsd<RSSE, M64>&);

        void exec(const Mulsd<RSSE, RSSE>&);
        void exec(const Mulsd<RSSE, M64>&);

        void exec(const Divss<RSSE, RSSE>&);
        void exec(const Divss<RSSE, M32>&);
        void exec(const Divsd<RSSE, RSSE>&);
        void exec(const Divsd<RSSE, M64>&);

        void exec(const Comiss<RSSE, RSSE>&);
        void exec(const Comiss<RSSE, M32>&);
        void exec(const Comisd<RSSE, RSSE>&);
        void exec(const Comisd<RSSE, M64>&);
        void exec(const Ucomiss<RSSE, RSSE>&);
        void exec(const Ucomiss<RSSE, M32>&);
        void exec(const Ucomisd<RSSE, RSSE>&);
        void exec(const Ucomisd<RSSE, M64>&);

        void exec(const Cmpsd<RSSE, RSSE>&);
        void exec(const Cmpsd<RSSE, M64>&);

        void exec(const Cvtsi2ss<RSSE, RM32>&);
        void exec(const Cvtsi2ss<RSSE, RM64>&);
        void exec(const Cvtsi2sd<RSSE, RM32>&);
        void exec(const Cvtsi2sd<RSSE, RM64>&);

        void exec(const Cvtss2sd<RSSE, RSSE>&);
        void exec(const Cvtss2sd<RSSE, M32>&);

        void exec(const Cvttsd2si<R32, RSSE>&);
        void exec(const Cvttsd2si<R32, M64>&);
        void exec(const Cvttsd2si<R64, RSSE>&);
        void exec(const Cvttsd2si<R64, M64>&);

        void exec(const Pand<RSSE, RMSSE>&);
        void exec(const Pandn<RSSE, RMSSE>&);
        void exec(const Por<RSSE, RMSSE>&);

        void exec(const Andpd<RSSE, RMSSE>&);
        void exec(const Andnpd<RSSE, RMSSE>&);
        void exec(const Orpd<RSSE, RMSSE>&);
        void exec(const Xorpd<RSSE, RMSSE>&);
        void exec(const Shufpd<RSSE, RMSSE, Imm>&);

        void exec(const Movlps<RSSE, M64>&);
        void exec(const Movhps<RSSE, M64>&);

        void exec(const Punpcklbw<RSSE, RSSE>&);
        void exec(const Punpcklwd<RSSE, RSSE>&);
        void exec(const Punpckldq<RSSE, RSSE>&);
        void exec(const Punpcklqdq<RSSE, RSSE>&);

        void exec(const Punpckhbw<RSSE, RSSE>&);
        void exec(const Punpckhwd<RSSE, RSSE>&);
        void exec(const Punpckhdq<RSSE, RSSE>&);
        void exec(const Punpckhqdq<RSSE, RSSE>&);

        void exec(const Pshufb<RSSE, RMSSE>&);
        void exec(const Pshufd<RSSE, RMSSE, Imm>&);

        void exec(const Pcmpeqb<RSSE, RMSSE>&);
        void exec(const Pcmpeqw<RSSE, RMSSE>&);
        void exec(const Pcmpeqd<RSSE, RMSSE>&);
        void exec(const Pcmpeqq<RSSE, RMSSE>&);

        void exec(const Pcmpgtb<RSSE, RMSSE>&);
        void exec(const Pcmpgtw<RSSE, RMSSE>&);
        void exec(const Pcmpgtd<RSSE, RMSSE>&);
        void exec(const Pcmpgtq<RSSE, RMSSE>&);

        void exec(const Pmovmskb<R32, RSSE>&);

        void exec(const Paddb<RSSE, RMSSE>&);
        void exec(const Paddw<RSSE, RMSSE>&);
        void exec(const Paddd<RSSE, RMSSE>&);
        void exec(const Paddq<RSSE, RMSSE>&);

        void exec(const Psubb<RSSE, RMSSE>&);
        void exec(const Psubw<RSSE, RMSSE>&);
        void exec(const Psubd<RSSE, RMSSE>&);
        void exec(const Psubq<RSSE, RMSSE>&);

        void exec(const Pmaxub<RSSE, RMSSE>&);
        void exec(const Pminub<RSSE, RMSSE>&);

        void exec(const Ptest<RSSE, RMSSE>&);

        void exec(const Psllw<RSSE, Imm>&);
        void exec(const Pslld<RSSE, Imm>&);
        void exec(const Psllq<RSSE, Imm>&);
        void exec(const Psrlw<RSSE, Imm>&);
        void exec(const Psrld<RSSE, Imm>&);
        void exec(const Psrlq<RSSE, Imm>&);

        void exec(const Pslldq<RSSE, Imm>&);
        void exec(const Psrldq<RSSE, Imm>&);
        
        void exec(const Pcmpistri<RSSE, RMSSE, Imm>&);

        void exec(const Packuswb<RSSE, RMSSE>&);
        void exec(const Packusdw<RSSE, RMSSE>&);
        void exec(const Packsswb<RSSE, RMSSE>&);
        void exec(const Packssdw<RSSE, RMSSE>&);
        
        void exec(const Rdtsc&);

        void exec(const Cpuid&);
        void exec(const Xgetbv&);

        void exec(const Fxsave<M64>&);
        void exec(const Fxrstor<M64>&);

        void exec(const Fwait&);

        void exec(const Rdpkru&);
        void exec(const Wrpkru&);

    };

}

#endif
