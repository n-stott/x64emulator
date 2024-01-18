#ifndef CPU_H
#define CPU_H

#include "instructionhandler.h"
#include "interpreter/registers.h"
#include "interpreter/flags.h"
#include "interpreter/x87.h"

namespace x64 {

    class VM;
    class Mmu;
    struct X86Instruction;

    class Cpu final : public InstructionHandler {
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

        template<typename Dst, typename Src>
        void execCmovImpl(Cond cond, Dst dst, Src src);

        void execFcmovImpl(Cond cond, ST dst, ST src);

        template<typename Dst>
        void execCmpxchg32Impl(Dst dst, u32 src);

        template<typename Dst>
        void execCmpxchg64Impl(Dst dst, u64 src);

    public:
        void exec(const Add<R8, R8>&) override;
        void exec(const Add<R8, Imm>&) override;
        void exec(const Add<R8, M8>&) override;
        void exec(const Add<M8, R8>&) override;
        void exec(const Add<M8, Imm>&) override;
        void exec(const Add<R16, R16>&) override;
        void exec(const Add<R16, Imm>&) override;
        void exec(const Add<R16, M16>&) override;
        void exec(const Add<M16, R16>&) override;
        void exec(const Add<M16, Imm>&) override;
        void exec(const Add<R32, R32>&) override;
        void exec(const Add<R32, Imm>&) override;
        void exec(const Add<R32, M32>&) override;
        void exec(const Add<M32, R32>&) override;
        void exec(const Add<M32, Imm>&) override;
        void exec(const Add<R64, R64>&) override;
        void exec(const Add<R64, Imm>&) override;
        void exec(const Add<R64, M64>&) override;
        void exec(const Add<M64, R64>&) override;
        void exec(const Add<M64, Imm>&) override;

        void exec(const Adc<R32, R32>&) override;
        void exec(const Adc<R32, Imm>&) override;
        void exec(const Adc<R32, M32>&) override;
        void exec(const Adc<M32, R32>&) override;
        void exec(const Adc<M32, Imm>&) override;
        void exec(const Adc<R64, R64>&) override;
        void exec(const Adc<R64, Imm>&) override;
        void exec(const Adc<R64, M64>&) override;
        void exec(const Adc<M64, R64>&) override;
        void exec(const Adc<M64, Imm>&) override;

        void exec(const Sub<R8, R8>&) override;
        void exec(const Sub<R8, Imm>&) override;
        void exec(const Sub<R8, M8>&) override;
        void exec(const Sub<M8, R8>&) override;
        void exec(const Sub<M8, Imm>&) override;
        void exec(const Sub<R16, R16>&) override;
        void exec(const Sub<R16, Imm>&) override;
        void exec(const Sub<R16, M16>&) override;
        void exec(const Sub<M16, R16>&) override;
        void exec(const Sub<M16, Imm>&) override;
        void exec(const Sub<R32, R32>&) override;
        void exec(const Sub<R32, Imm>&) override;
        void exec(const Sub<R32, M32>&) override;
        void exec(const Sub<M32, R32>&) override;
        void exec(const Sub<M32, Imm>&) override;
        void exec(const Sub<R64, R64>&) override;
        void exec(const Sub<R64, Imm>&) override;
        void exec(const Sub<R64, M64>&) override;
        void exec(const Sub<M64, R64>&) override;
        void exec(const Sub<M64, Imm>&) override;

        void exec(const Sbb<R8, R8>&) override;
        void exec(const Sbb<R8, Imm>&) override;
        void exec(const Sbb<R8, M8>&) override;
        void exec(const Sbb<M8, R8>&) override;
        void exec(const Sbb<M8, Imm>&) override;
        void exec(const Sbb<R16, R16>&) override;
        void exec(const Sbb<R16, Imm>&) override;
        void exec(const Sbb<R16, M16>&) override;
        void exec(const Sbb<M16, R16>&) override;
        void exec(const Sbb<M16, Imm>&) override;
        void exec(const Sbb<R32, R32>&) override;
        void exec(const Sbb<R32, Imm>&) override;
        void exec(const Sbb<R32, M32>&) override;
        void exec(const Sbb<M32, R32>&) override;
        void exec(const Sbb<M32, Imm>&) override;
        void exec(const Sbb<R64, R64>&) override;
        void exec(const Sbb<R64, Imm>&) override;
        void exec(const Sbb<R64, M64>&) override;
        void exec(const Sbb<M64, R64>&) override;
        void exec(const Sbb<M64, Imm>&) override;

        void exec(const Neg<R8>&) override;
        void exec(const Neg<M8>&) override;
        void exec(const Neg<R16>&) override;
        void exec(const Neg<M16>&) override;
        void exec(const Neg<R32>&) override;
        void exec(const Neg<M32>&) override;
        void exec(const Neg<R64>&) override;
        void exec(const Neg<M64>&) override;

        void exec(const Mul<R32>&) override;
        void exec(const Mul<M32>&) override;
        void exec(const Mul<R64>&) override;
        void exec(const Mul<M64>&) override;

        void exec(const Imul1<R32>&) override;
        void exec(const Imul1<M32>&) override;
        void exec(const Imul2<R32, R32>&) override;
        void exec(const Imul2<R32, M32>&) override;
        void exec(const Imul3<R32, R32, Imm>&) override;
        void exec(const Imul3<R32, M32, Imm>&) override;
        void exec(const Imul1<R64>&) override;
        void exec(const Imul1<M64>&) override;
        void exec(const Imul2<R64, R64>&) override;
        void exec(const Imul2<R64, M64>&) override;
        void exec(const Imul3<R64, R64, Imm>&) override;
        void exec(const Imul3<R64, M64, Imm>&) override;

        void exec(const Div<R32>&) override;
        void exec(const Div<M32>&) override;
        void exec(const Div<R64>&) override;
        void exec(const Div<M64>&) override;

        void exec(const Idiv<R32>&) override;
        void exec(const Idiv<M32>&) override;
        void exec(const Idiv<R64>&) override;
        void exec(const Idiv<M64>&) override;

        void exec(const And<R8, R8>&) override;
        void exec(const And<R8, Imm>&) override;
        void exec(const And<R8, M8>&) override;
        void exec(const And<M8, R8>&) override;
        void exec(const And<M8, Imm>&) override;
        void exec(const And<R16, Imm>&) override;
        void exec(const And<R16, R16>&) override;
        void exec(const And<R16, M16>&) override;
        void exec(const And<M16, Imm>&) override;
        void exec(const And<M16, R16>&) override;
        void exec(const And<R32, R32>&) override;
        void exec(const And<R32, Imm>&) override;
        void exec(const And<R32, M32>&) override;
        void exec(const And<M32, R32>&) override;
        void exec(const And<M32, Imm>&) override;
        void exec(const And<R64, R64>&) override;
        void exec(const And<R64, Imm>&) override;
        void exec(const And<R64, M64>&) override;
        void exec(const And<M64, R64>&) override;
        void exec(const And<M64, Imm>&) override;

        void exec(const Or<R8, R8>&) override;
        void exec(const Or<R8, Imm>&) override;
        void exec(const Or<R8, M8>&) override;
        void exec(const Or<M8, R8>&) override;
        void exec(const Or<M8, Imm>&) override;
        void exec(const Or<R16, R16>&) override;
        void exec(const Or<R16, Imm>&) override;
        void exec(const Or<R16, M16>&) override;
        void exec(const Or<M16, R16>&) override;
        void exec(const Or<R32, R32>&) override;
        void exec(const Or<R32, Imm>&) override;
        void exec(const Or<R32, M32>&) override;
        void exec(const Or<M32, R32>&) override;
        void exec(const Or<M32, Imm>&) override;
        void exec(const Or<R64, R64>&) override;
        void exec(const Or<R64, Imm>&) override;
        void exec(const Or<R64, M64>&) override;
        void exec(const Or<M64, R64>&) override;
        void exec(const Or<M64, Imm>&) override;

        void exec(const Xor<R8, R8>&) override;
        void exec(const Xor<R8, Imm>&) override;
        void exec(const Xor<R8, M8>&) override;
        void exec(const Xor<M8, Imm>&) override;
        void exec(const Xor<R16, Imm>&) override;
        void exec(const Xor<R32, R32>&) override;
        void exec(const Xor<R32, Imm>&) override;
        void exec(const Xor<R32, M32>&) override;
        void exec(const Xor<M32, R32>&) override;
        void exec(const Xor<R64, R64>&) override;
        void exec(const Xor<R64, Imm>&) override;
        void exec(const Xor<R64, M64>&) override;
        void exec(const Xor<M64, R64>&) override;

        void exec(const Not<R8>&) override;
        void exec(const Not<M8>&) override;
        void exec(const Not<R16>&) override;
        void exec(const Not<M16>&) override;
        void exec(const Not<R32>&) override;
        void exec(const Not<M32>&) override;
        void exec(const Not<R64>&) override;
        void exec(const Not<M64>&) override;

        void exec(const Xchg<R16, R16>&) override;
        void exec(const Xchg<R32, R32>&) override;
        void exec(const Xchg<M32, R32>&) override;
        void exec(const Xchg<R64, R64>&) override;
        void exec(const Xchg<M64, R64>&) override;

        void exec(const Xadd<R16, R16>&) override;
        void exec(const Xadd<R32, R32>&) override;
        void exec(const Xadd<M32, R32>&) override;
        void exec(const Xadd<R64, R64>&) override;
        void exec(const Xadd<M64, R64>&) override;

        void exec(const Mov<R8, R8>&) override;
        void exec(const Mov<R8, Imm>&) override;
        void exec(const Mov<R8, M8>&) override;
        void exec(const Mov<M8, R8>&) override;
        void exec(const Mov<M8, Imm>&) override;
        void exec(const Mov<R16, R16>&) override;
        void exec(const Mov<R16, Imm>&) override;
        void exec(const Mov<R16, M16>&) override;
        void exec(const Mov<M16, R16>&) override;
        void exec(const Mov<M16, Imm>&) override;
        void exec(const Mov<R32, R32>&) override;
        void exec(const Mov<R32, Imm>&) override;
        void exec(const Mov<R32, M32>&) override;
        void exec(const Mov<M32, R32>&) override;
        void exec(const Mov<M32, Imm>&) override;
        void exec(const Mov<R64, R64>&) override;
        void exec(const Mov<R64, Imm>&) override;
        void exec(const Mov<R64, M64>&) override;
        void exec(const Mov<M64, R64>&) override;
        void exec(const Mov<M64, Imm>&) override;
        void exec(const Mov<RSSE, RSSE>&) override;
        void exec(const Mov<RSSE, MSSE>&) override;
        void exec(const Mov<MSSE, RSSE>&) override;

        void exec(const Movsx<R32, R8>&) override;
        void exec(const Movsx<R32, M8>&) override;
        void exec(const Movsx<R64, R8>&) override;
        void exec(const Movsx<R64, M8>&) override;
        void exec(const Movsx<R32, R16>&) override;
        void exec(const Movsx<R32, M16>&) override;
        void exec(const Movsx<R64, R16>&) override;
        void exec(const Movsx<R64, M16>&) override;
        void exec(const Movsx<R32, R32>&) override;
        void exec(const Movsx<R32, M32>&) override;
        void exec(const Movsx<R64, R32>&) override;
        void exec(const Movsx<R64, M32>&) override;

        void exec(const Movzx<R16, R8>&) override;
        void exec(const Movzx<R32, R8>&) override;
        void exec(const Movzx<R32, R16>&) override;
        void exec(const Movzx<R32, M8>&) override;
        void exec(const Movzx<R32, M16>&) override;

        void exec(const Lea<R32, B>&) override;
        void exec(const Lea<R32, BD>&) override;
        void exec(const Lea<R32, BIS>&) override;
        void exec(const Lea<R32, ISD>&) override;
        void exec(const Lea<R32, BISD>&) override;
        void exec(const Lea<R64, B>&) override;
        void exec(const Lea<R64, BD>&) override;
        void exec(const Lea<R64, BIS>&) override;
        void exec(const Lea<R64, ISD>&) override;
        void exec(const Lea<R64, BISD>&) override;

        void exec(const Push<Imm>&) override;
        void exec(const Push<R32>&) override;
        void exec(const Push<M32>&) override;
        void exec(const Push<R64>&) override;
        void exec(const Push<M64>&) override;

        void exec(const Pop<R32>&) override;
        void exec(const Pop<R64>&) override;

        void exec(const CallDirect&) override;
        void exec(const CallIndirect<R32>&) override;
        void exec(const CallIndirect<M32>&) override;
        void exec(const CallIndirect<R64>&) override;
        void exec(const CallIndirect<M64>&) override;
        void exec(const Ret<>&) override;
        void exec(const Ret<Imm>&) override;

        void exec(const Leave&) override;
        void exec(const Halt&) override;
        void exec(const Nop&) override;
        void exec(const Ud2&) override;
        void exec(const Syscall&) override;
        void exec(const NotParsed&) override;
        void exec(const Unknown&) override;

        void exec(const Cdq&) override;
        void exec(const Cqo&) override;

        void exec(const Inc<R8>&) override;
        void exec(const Inc<M8>&) override;
        void exec(const Inc<M16>&) override;
        void exec(const Inc<R32>&) override;
        void exec(const Inc<M32>&) override;
        void exec(const Inc<R64>&) override;
        void exec(const Inc<M64>&) override;

        void exec(const Dec<R8>&) override;
        void exec(const Dec<M16>&) override;
        void exec(const Dec<R32>&) override;
        void exec(const Dec<M32>&) override;

        void exec(const Shr<R8, Imm>&) override;
        void exec(const Shr<R16, Imm>&) override;
        void exec(const Shr<R32, R8>&) override;
        void exec(const Shr<M32, R8>&) override;
        void exec(const Shr<R32, Imm>&) override;
        void exec(const Shr<M32, Imm>&) override;
        void exec(const Shr<R64, R8>&) override;
        void exec(const Shr<M64, R8>&) override;
        void exec(const Shr<R64, Imm>&) override;
        void exec(const Shr<M64, Imm>&) override;

        void exec(const Shl<R32, R8>&) override;
        void exec(const Shl<M32, R8>&) override;
        void exec(const Shl<R32, Imm>&) override;
        void exec(const Shl<M32, Imm>&) override;
        void exec(const Shl<R64, R8>&) override;
        void exec(const Shl<M64, R8>&) override;
        void exec(const Shl<R64, Imm>&) override;
        void exec(const Shl<M64, Imm>&) override;

        void exec(const Shld<R32, R32, R8>&) override;
        void exec(const Shld<R32, R32, Imm>&) override;

        void exec(const Shrd<R32, R32, R8>&) override;
        void exec(const Shrd<R32, R32, Imm>&) override;

        void exec(const Sar<R32, R8>&) override;
        void exec(const Sar<R32, Imm>&) override;
        void exec(const Sar<M32, Imm>&) override;
        void exec(const Sar<R64, R8>&) override;
        void exec(const Sar<R64, Imm>&) override;
        void exec(const Sar<M64, Imm>&) override;

        void exec(const Rol<R32, R8>&) override;
        void exec(const Rol<R32, Imm>&) override;
        void exec(const Rol<M32, Imm>&) override;
        void exec(const Rol<R64, R8>&) override;
        void exec(const Rol<R64, Imm>&) override;
        void exec(const Rol<M64, Imm>&) override;

        void exec(const Ror<R32, R8>&) override;
        void exec(const Ror<R32, Imm>&) override;
        void exec(const Ror<M32, Imm>&) override;
        void exec(const Ror<R64, R8>&) override;
        void exec(const Ror<R64, Imm>&) override;
        void exec(const Ror<M64, Imm>&) override;

        void exec(const Tzcnt<R16, R16>&) override;
        void exec(const Tzcnt<R16, M16>&) override;
        void exec(const Tzcnt<R32, R32>&) override;
        void exec(const Tzcnt<R32, M32>&) override;
        void exec(const Tzcnt<R64, R64>&) override;
        void exec(const Tzcnt<R64, M64>&) override;

        void exec(const Bt<R16, R16>&) override;
        void exec(const Bt<R16, Imm>&) override;
        void exec(const Bt<R32, R32>&) override;
        void exec(const Bt<R32, Imm>&) override;
        void exec(const Bt<R64, R64>&) override;
        void exec(const Bt<R64, Imm>&) override;
        void exec(const Bt<M16, R16>&) override;
        void exec(const Bt<M16, Imm>&) override;
        void exec(const Bt<M32, R32>&) override;
        void exec(const Bt<M32, Imm>&) override;
        void exec(const Bt<M64, R64>&) override;
        void exec(const Bt<M64, Imm>&) override;

        void exec(const Btr<R16, R16>&) override;
        void exec(const Btr<R16, Imm>&) override;
        void exec(const Btr<R32, R32>&) override;
        void exec(const Btr<R32, Imm>&) override;
        void exec(const Btr<R64, R64>&) override;
        void exec(const Btr<R64, Imm>&) override;
        void exec(const Btr<M16, R16>&) override;
        void exec(const Btr<M16, Imm>&) override;
        void exec(const Btr<M32, R32>&) override;
        void exec(const Btr<M32, Imm>&) override;
        void exec(const Btr<M64, R64>&) override;
        void exec(const Btr<M64, Imm>&) override;

        void exec(const Btc<R16, R16>&) override;
        void exec(const Btc<R16, Imm>&) override;
        void exec(const Btc<R32, R32>&) override;
        void exec(const Btc<R32, Imm>&) override;
        void exec(const Btc<R64, R64>&) override;
        void exec(const Btc<R64, Imm>&) override;
        void exec(const Btc<M16, R16>&) override;
        void exec(const Btc<M16, Imm>&) override;
        void exec(const Btc<M32, R32>&) override;
        void exec(const Btc<M32, Imm>&) override;
        void exec(const Btc<M64, R64>&) override;
        void exec(const Btc<M64, Imm>&) override;

        void exec(const Test<R8, R8>&) override;
        void exec(const Test<R8, Imm>&) override;
        void exec(const Test<M8, R8>&) override;
        void exec(const Test<M8, Imm>&) override;
        void exec(const Test<R16, R16>&) override;
        void exec(const Test<R16, Imm>&) override;
        void exec(const Test<M16, R16>&) override;
        void exec(const Test<M16, Imm>&) override;
        void exec(const Test<R32, R32>&) override;
        void exec(const Test<R32, Imm>&) override;
        void exec(const Test<M32, R32>&) override;
        void exec(const Test<M32, Imm>&) override;
        void exec(const Test<R64, R64>&) override;
        void exec(const Test<R64, Imm>&) override;
        void exec(const Test<M64, R64>&) override;
        void exec(const Test<M64, Imm>&) override;

        void exec(const Cmp<R8, R8>&) override;
        void exec(const Cmp<R8, Imm>&) override;
        void exec(const Cmp<R8, M8>&) override;
        void exec(const Cmp<M8, R8>&) override;
        void exec(const Cmp<M8, Imm>&) override;
        void exec(const Cmp<R16, R16>&) override;
        void exec(const Cmp<R16, Imm>&) override;
        void exec(const Cmp<R16, M16>&) override;
        void exec(const Cmp<M16, R16>&) override;
        void exec(const Cmp<M16, Imm>&) override;
        void exec(const Cmp<R32, R32>&) override;
        void exec(const Cmp<R32, Imm>&) override;
        void exec(const Cmp<R32, M32>&) override;
        void exec(const Cmp<M32, R32>&) override;
        void exec(const Cmp<M32, Imm>&) override;
        void exec(const Cmp<R64, R64>&) override;
        void exec(const Cmp<R64, Imm>&) override;
        void exec(const Cmp<R64, M64>&) override;
        void exec(const Cmp<M64, R64>&) override;
        void exec(const Cmp<M64, Imm>&) override;

        void exec(const Cmpxchg<R8, R8>&) override;
        void exec(const Cmpxchg<M8, R8>&) override;
        void exec(const Cmpxchg<R16, R16>&) override;
        void exec(const Cmpxchg<M16, R16>&) override;
        void exec(const Cmpxchg<R32, R32>&) override;
        void exec(const Cmpxchg<M32, R32>&) override;
        void exec(const Cmpxchg<R64, R64>&) override;
        void exec(const Cmpxchg<M64, R64>&) override;

        void exec(const Set<Cond::AE, R8>&) override;
        void exec(const Set<Cond::AE, M8>&) override;
        void exec(const Set<Cond::A, R8>&) override;
        void exec(const Set<Cond::A,M8>&) override;
        void exec(const Set<Cond::B, R8>&) override;
        void exec(const Set<Cond::B, M8>&) override;
        void exec(const Set<Cond::BE, R8>&) override;
        void exec(const Set<Cond::BE, M8>&) override;
        void exec(const Set<Cond::E, R8>&) override;
        void exec(const Set<Cond::E, M8>&) override;
        void exec(const Set<Cond::G, R8>&) override;
        void exec(const Set<Cond::G, M8>&) override;
        void exec(const Set<Cond::GE, R8>&) override;
        void exec(const Set<Cond::GE, M8>&) override;
        void exec(const Set<Cond::L, R8>&) override;
        void exec(const Set<Cond::L, M8>&) override;
        void exec(const Set<Cond::LE, R8>&) override;
        void exec(const Set<Cond::LE, M8>&) override;
        void exec(const Set<Cond::NE, R8>&) override;
        void exec(const Set<Cond::NE, M8>&) override;
        void exec(const Set<Cond::NO, R8>&) override;
        void exec(const Set<Cond::NO, M8>&) override;
        void exec(const Set<Cond::NS, R8>&) override;
        void exec(const Set<Cond::NS, M8>&) override;
        void exec(const Set<Cond::O, R8>&) override;
        void exec(const Set<Cond::O, M8>&) override;
        void exec(const Set<Cond::S, R8>&) override;
        void exec(const Set<Cond::S, M8>&) override;

        void exec(const Jmp<R32>&) override;
        void exec(const Jmp<R64>&) override;
        void exec(const Jmp<u32>&) override;
        void exec(const Jmp<M32>&) override;
        void exec(const Jmp<M64>&) override;
        void exec(const Jcc<Cond::NE>&) override;
        void exec(const Jcc<Cond::E>&) override;
        void exec(const Jcc<Cond::AE>&) override;
        void exec(const Jcc<Cond::BE>&) override;
        void exec(const Jcc<Cond::GE>&) override;
        void exec(const Jcc<Cond::LE>&) override;
        void exec(const Jcc<Cond::A>&) override;
        void exec(const Jcc<Cond::B>&) override;
        void exec(const Jcc<Cond::G>&) override;
        void exec(const Jcc<Cond::L>&) override;
        void exec(const Jcc<Cond::S>&) override;
        void exec(const Jcc<Cond::NS>&) override;
        void exec(const Jcc<Cond::O>&) override;
        void exec(const Jcc<Cond::NO>&) override;
        void exec(const Jcc<Cond::P>&) override;
        void exec(const Jcc<Cond::NP>&) override;

        void exec(const Bsr<R32, R32>&) override;
        void exec(const Bsr<R64, R64>&) override;

        void exec(const Bsf<R32, R32>&) override;
        void exec(const Bsf<R64, R64>&) override;

        void exec(const Cld&) override;
        void exec(const Std&) override;

        void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&) override;
        void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&) override;
        void exec(const Rep<Movs<M64, M64>>&) override;
        
        void exec(const Rep<Cmps<M8, M8>>&) override;
        
        void exec(const Rep<Stos<M32, R32>>&) override;
        void exec(const Rep<Stos<M64, R64>>&) override;

        void exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>&) override;

        void exec(const Cmov<Cond::AE, R32, R32>&) override;
        void exec(const Cmov<Cond::AE, R32, M32>&) override;
        void exec(const Cmov<Cond::A, R32, R32>&) override;
        void exec(const Cmov<Cond::A, R32, M32>&) override;
        void exec(const Cmov<Cond::BE, R32, R32>&) override;
        void exec(const Cmov<Cond::BE, R32, M32>&) override;
        void exec(const Cmov<Cond::B, R32, R32>&) override;
        void exec(const Cmov<Cond::B, R32, M32>&) override;
        void exec(const Cmov<Cond::E, R32, R32>&) override;
        void exec(const Cmov<Cond::E, R32, M32>&) override;
        void exec(const Cmov<Cond::GE, R32, R32>&) override;
        void exec(const Cmov<Cond::GE, R32, M32>&) override;
        void exec(const Cmov<Cond::G, R32, R32>&) override;
        void exec(const Cmov<Cond::G, R32, M32>&) override;
        void exec(const Cmov<Cond::LE, R32, R32>&) override;
        void exec(const Cmov<Cond::LE, R32, M32>&) override;
        void exec(const Cmov<Cond::L, R32, R32>&) override;
        void exec(const Cmov<Cond::L, R32, M32>&) override;
        void exec(const Cmov<Cond::NE, R32, R32>&) override;
        void exec(const Cmov<Cond::NE, R32, M32>&) override;
        void exec(const Cmov<Cond::NS, R32, R32>&) override;
        void exec(const Cmov<Cond::NS, R32, M32>&) override;
        void exec(const Cmov<Cond::S, R32, R32>&) override;
        void exec(const Cmov<Cond::S, R32, M32>&) override;
        void exec(const Cmov<Cond::AE, R64, R64>&) override;
        void exec(const Cmov<Cond::AE, R64, M64>&) override;
        void exec(const Cmov<Cond::A, R64, R64>&) override;
        void exec(const Cmov<Cond::A, R64, M64>&) override;
        void exec(const Cmov<Cond::BE, R64, R64>&) override;
        void exec(const Cmov<Cond::BE, R64, M64>&) override;
        void exec(const Cmov<Cond::B, R64, R64>&) override;
        void exec(const Cmov<Cond::B, R64, M64>&) override;
        void exec(const Cmov<Cond::E, R64, R64>&) override;
        void exec(const Cmov<Cond::E, R64, M64>&) override;
        void exec(const Cmov<Cond::GE, R64, R64>&) override;
        void exec(const Cmov<Cond::GE, R64, M64>&) override;
        void exec(const Cmov<Cond::G, R64, R64>&) override;
        void exec(const Cmov<Cond::G, R64, M64>&) override;
        void exec(const Cmov<Cond::LE, R64, R64>&) override;
        void exec(const Cmov<Cond::LE, R64, M64>&) override;
        void exec(const Cmov<Cond::L, R64, R64>&) override;
        void exec(const Cmov<Cond::L, R64, M64>&) override;
        void exec(const Cmov<Cond::NE, R64, R64>&) override;
        void exec(const Cmov<Cond::NE, R64, M64>&) override;
        void exec(const Cmov<Cond::NS, R64, R64>&) override;
        void exec(const Cmov<Cond::NS, R64, M64>&) override;
        void exec(const Cmov<Cond::S, R64, R64>&) override;
        void exec(const Cmov<Cond::S, R64, M64>&) override;

        void exec(const Cwde&) override;
        void exec(const Cdqe&) override;

        void exec(const Pxor<RSSE, RSSE>&) override;
        void exec(const Pxor<RSSE, MSSE>&) override;

        void exec(const Movaps<RSSE, RSSE>&) override;
        void exec(const Movaps<MSSE, RSSE>&) override;
        void exec(const Movaps<RSSE, MSSE>&) override;
        void exec(const Movaps<MSSE, MSSE>&) override;

        void exec(const Movd<RSSE, R32>&) override;
        void exec(const Movd<R32, RSSE>&) override;

        void exec(const Movq<RSSE, R64>&) override;
        void exec(const Movq<R64, RSSE>&) override;
        void exec(const Movq<RSSE, M64>&) override;
        void exec(const Movq<M64, RSSE>&) override;

        void exec(const Fldz&) override;
        void exec(const Fld1&) override;
        void exec(const Fld<ST>&) override;
        void exec(const Fld<M32>&) override;
        void exec(const Fld<M64>&) override;
        void exec(const Fld<M80>&) override;
        void exec(const Fild<M16>&) override;
        void exec(const Fild<M32>&) override;
        void exec(const Fild<M64>&) override;
        void exec(const Fstp<ST>&) override;
        void exec(const Fstp<M32>&) override;
        void exec(const Fstp<M64>&) override;
        void exec(const Fstp<M80>&) override;
        void exec(const Fistp<M16>&) override;
        void exec(const Fistp<M32>&) override;
        void exec(const Fistp<M64>&) override;
        void exec(const Fxch<ST>&) override;

        void exec(const Faddp<ST>&) override;
        void exec(const Fsubrp<ST>&) override;
        void exec(const Fmul1<M32>&) override;
        void exec(const Fmul1<M64>&) override;
        void exec(const Fdiv<ST, ST>&) override;
        void exec(const Fdivp<ST, ST>&) override;

        void exec(const Fcomi<ST>&) override;
        void exec(const Fucomi<ST>&) override;
        void exec(const Frndint&) override;

        void exec(const Fcmov<Cond::B, ST>&) override;
        void exec(const Fcmov<Cond::BE, ST>&) override;
        void exec(const Fcmov<Cond::E, ST>&) override;
        void exec(const Fcmov<Cond::NB, ST>&) override;
        void exec(const Fcmov<Cond::NBE, ST>&) override;
        void exec(const Fcmov<Cond::NE, ST>&) override;
        void exec(const Fcmov<Cond::NU, ST>&) override;
        void exec(const Fcmov<Cond::U, ST>&) override;

        void exec(const Fnstcw<M16>&) override;
        void exec(const Fldcw<M16>&) override;

        void exec(const Fnstsw<R16>&) override;
        void exec(const Fnstsw<M16>&) override;

        void exec(const Fnstenv<M224>&) override;
        void exec(const Fldenv<M224>&) override;

        void exec(const Movss<RSSE, M32>&) override;
        void exec(const Movss<M32, RSSE>&) override;

        void exec(const Movsd<RSSE, M64>&) override;
        void exec(const Movsd<M64, RSSE>&) override;
        
        void exec(const Addss<RSSE, RSSE>&) override;
        void exec(const Addss<RSSE, M32>&) override;
        void exec(const Addsd<RSSE, RSSE>&) override;
        void exec(const Addsd<RSSE, M64>&) override;

        void exec(const Subsd<RSSE, RSSE>&) override;
        void exec(const Subsd<RSSE, M64>&) override;
        void exec(const Subss<RSSE, RSSE>&) override;
        void exec(const Subss<RSSE, M32>&) override;

        void exec(const Mulsd<RSSE, RSSE>&) override;
        void exec(const Mulsd<RSSE, M64>&) override;

        void exec(const Divsd<RSSE, RSSE>&) override;
        void exec(const Divsd<RSSE, M64>&) override;

        void exec(const Comiss<RSSE, RSSE>&) override;
        void exec(const Comiss<RSSE, M32>&) override;
        void exec(const Comisd<RSSE, RSSE>&) override;
        void exec(const Comisd<RSSE, M64>&) override;
        void exec(const Ucomiss<RSSE, RSSE>&) override;
        void exec(const Ucomiss<RSSE, M32>&) override;
        void exec(const Ucomisd<RSSE, RSSE>&) override;
        void exec(const Ucomisd<RSSE, M64>&) override;

        void exec(const Cmpsd<RSSE, RSSE>&) override;
        void exec(const Cmpsd<RSSE, M64>&) override;

        void exec(const Cvtsi2sd<RSSE, R32>&) override;
        void exec(const Cvtsi2sd<RSSE, M32>&) override;
        void exec(const Cvtsi2sd<RSSE, R64>&) override;
        void exec(const Cvtsi2sd<RSSE, M64>&) override;
        void exec(const Cvtss2sd<RSSE, RSSE>&) override;
        void exec(const Cvtss2sd<RSSE, M32>&) override;

        void exec(const Cvttsd2si<R32, RSSE>&) override;
        void exec(const Cvttsd2si<R32, M64>&) override;
        void exec(const Cvttsd2si<R64, RSSE>&) override;
        void exec(const Cvttsd2si<R64, M64>&) override;

        void exec(const Pand<RSSE, RSSE>&) override;
        void exec(const Pand<RSSE, MSSE>&) override;
        void exec(const Por<RSSE, RSSE>&) override;
        void exec(const Por<RSSE, MSSE>&) override;

        void exec(const Andpd<RSSE, RSSE>&) override;
        void exec(const Andnpd<RSSE, RSSE>&) override;
        void exec(const Orpd<RSSE, RSSE>&) override;
        void exec(const Xorpd<RSSE, RSSE>&) override;

        void exec(const Movlps<RSSE, M64>&) override;
        void exec(const Movhps<RSSE, M64>&) override;

        void exec(const Punpcklbw<RSSE, RSSE>&) override;
        void exec(const Punpcklwd<RSSE, RSSE>&) override;
        void exec(const Punpckldq<RSSE, RSSE>&) override;
        void exec(const Punpcklqdq<RSSE, RSSE>&) override;

        void exec(const Pshufb<RSSE, RSSE>&) override;
        void exec(const Pshufb<RSSE, MSSE>&) override;
        void exec(const Pshufd<RSSE, RSSE, Imm>&) override;
        void exec(const Pshufd<RSSE, MSSE, Imm>&) override;

        void exec(const Pcmpeqb<RSSE, RSSE>&) override;
        void exec(const Pcmpeqb<RSSE, MSSE>&) override;

        void exec(const Pmovmskb<R32, RSSE>&) override;

        void exec(const Psubb<RSSE, RSSE>&) override;
        void exec(const Psubb<RSSE, MSSE>&) override;

        void exec(const Pminub<RSSE, RSSE>&) override;
        void exec(const Pminub<RSSE, MSSE>&) override;

        void exec(const Ptest<RSSE, RSSE>&) override;
        void exec(const Ptest<RSSE, MSSE>&) override;

        void exec(const Psllw<RSSE, Imm>&) override;
        void exec(const Pslld<RSSE, Imm>&) override;
        void exec(const Psllq<RSSE, Imm>&) override;
        void exec(const Psrlw<RSSE, Imm>&) override;
        void exec(const Psrld<RSSE, Imm>&) override;
        void exec(const Psrlq<RSSE, Imm>&) override;

        void exec(const Pslldq<RSSE, Imm>&) override;
        void exec(const Psrldq<RSSE, Imm>&) override;
        
        void exec(const Pcmpistri<RSSE, RSSE, Imm>&) override;
        void exec(const Pcmpistri<RSSE, MSSE, Imm>&) override;
        
        void exec(const Rdtsc&) override;

        void exec(const Cpuid&) override;
        void exec(const Xgetbv&) override;

        void exec(const Fxsave<M64>&) override;
        void exec(const Fxrstor<M64>&) override;

        void exec(const Fwait&) override;

        void exec(const Rdpkru&) override;
        void exec(const Wrpkru&) override;

        std::string functionName(const X86Instruction& instruction) const;

    };

}

#endif
