#ifndef CPU_H
#define CPU_H

#include "instructionhandler.h"
#include "interpreter/registers.h"
#include "interpreter/flags.h"

namespace x64 {

    class Interpreter;
    class Mmu;

    class Cpu final : public InstructionHandler {
    public:
        explicit Cpu(Interpreter* interpreter) : interpreter_(interpreter), mmu_(nullptr) { }
        void setMmu(Mmu* mmu) {
            mmu_ = mmu;
        }

    private:
        friend class Interpreter;
        friend class ExecutionContext;
        
        Interpreter* interpreter_;
        Mmu* mmu_;
        Flags flags_;
        Registers regs_;

        u8  get(R8 reg) const  { return regs_.get(reg); }
        u16 get(R16 reg) const { return regs_.get(reg); }
        u32 get(R32 reg) const { return regs_.get(reg); }
        u64 get(R64 reg) const { return regs_.get(reg); }
        Xmm get(RSSE reg) const { return regs_.get(reg); }

        template<typename T>
        T  get(Imm immediate) const;

        u8  get(Ptr8 reg) const;
        u16 get(Ptr16 reg) const;
        u32 get(Ptr32 reg) const;
        u64 get(Ptr64 reg) const;
        Xmm get(Ptr128 reg) const;

        u64 resolve(B addr) const { return regs_.resolve(addr); }
        u64 resolve(BD addr) const { return regs_.resolve(addr); }
        u64 resolve(BIS addr) const { return regs_.resolve(addr); }
        u64 resolve(ISD addr) const { return regs_.resolve(addr); }
        u64 resolve(BISD addr) const { return regs_.resolve(addr); }
        u64 resolve(SO addr) const { return regs_.resolve(addr); }

        template<Size size, typename Enc>
        Ptr<size> resolve(Addr<size, Enc> addr) const { return regs_.resolve(addr); }

        Ptr<Size::BYTE> resolve(const M8& m8) const { return regs_.resolve(m8); }
        Ptr<Size::WORD> resolve(const M16& m16) const { return regs_.resolve(m16); }
        Ptr<Size::DWORD> resolve(const M32& m32) const { return regs_.resolve(m32); }
        Ptr<Size::QWORD> resolve(const M64& m64) const { return regs_.resolve(m64); }
        Ptr<Size::XMMWORD> resolve(const MSSE& msse) const { return regs_.resolve(msse); }

        void set(R8 reg, u8 value) { regs_.set(reg, value); }
        void set(R16 reg, u16 value) { regs_.set(reg, value); }
        void set(R32 reg, u32 value) { regs_.set(reg, value); }
        void set(R64 reg, u64 value) { regs_.set(reg, value); }
        void set(RSSE reg, Xmm value) { regs_.set(reg, value); }

        void set(Ptr8 ptr, u8 value);
        void set(Ptr16 ptr, u16 value);
        void set(Ptr32 ptr, u32 value);
        void set(Ptr64 ptr, u64 value);
        void set(Ptr128 ptr, Xmm value);

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        void push64(u64 value);
        u8 pop8();
        u16 pop16();
        u32 pop32();
        u64 pop64();

        u8 execAdd8Impl(u8 dst, u8 src);
        u16 execAdd16Impl(u16 dst, u16 src);
        u32 execAdd32Impl(u32 dst, u32 src);
        u64 execAdd64Impl(u64 dst, u64 src);

        u8 execAdc8Impl(u8 dst, u8 src);
        u16 execAdc16Impl(u16 dst, u16 src);
        u32 execAdc32Impl(u32 dst, u32 src);

        u8 execSub8Impl(u8 src1, u8 src2);
        u16 execSub16Impl(u16 src1, u16 src2);
        u32 execSub32Impl(u32 src1, u32 src2);
        u64 execSub64Impl(u64 src1, u64 src2);

        u32 execSbb32Impl(u32 dst, u32 src);

        u32 execNeg32Impl(u32 dst);
        u64 execNeg64Impl(u64 dst);

        std::pair<u32, u32> execMul32(u32 src1, u32 src2);
        std::pair<u64, u64> execMul64(u64 src1, u64 src2);

        void execImul32(u32 src);
        u32 execImul32(u32 src1, u32 src2);
        void execImul64(u64 src);
        u64 execImul64(u64 src1, u64 src2);

        std::pair<u32, u32> execDiv32(u32 dividendUpper, u32 dividendLower, u32 divisor);
        std::pair<u64, u64> execDiv64(u64 dividendUpper, u64 dividendLower, u64 divisor);
        std::pair<u32, u32> execIdiv32(u32 dividendUpper, u32 dividendLower, u32 divisor);
        std::pair<u64, u64> execIdiv64(u64 dividendUpper, u64 dividendLower, u64 divisor);

        u8 execInc8Impl(u8 src);
        u16 execInc16Impl(u16 src);
        u32 execInc32Impl(u32 src);

        u32 execDec32Impl(u32 src);
        
        void execTest8Impl(u8 src1, u8 src2);
        void execTest16Impl(u16 src1, u16 src2);
        void execTest32Impl(u32 src1, u32 src2);
        void execTest64Impl(u64 src1, u64 src2);

        u8 execAnd8Impl(u8 dst, u8 src);
        u16 execAnd16Impl(u16 dst, u16 src);
        u32 execAnd32Impl(u32 dst, u32 src);
        u64 execAnd64Impl(u64 dst, u64 src);

        u8 execShr8Impl(u8 dst, u8 src);
        u16 execShr16Impl(u16 dst, u16 src);
        u32 execShr32Impl(u32 dst, u32 src);
        u64 execShr64Impl(u64 dst, u64 src);

        u32 execSar32Impl(i32 dst, u32 src);
        u64 execSar64Impl(i64 dst, u64 src);

        template<typename Dst>
        void execSet(Cond cond, Dst dst);

        template<typename Dst, typename Src>
        void execCmovImpl(Cond cond, Dst dst, Src src);

        u8 execOr8Impl(u8 dst, u8 src);
        u16 execOr16Impl(u16 dst, u16 src);
        u32 execOr32Impl(u32 dst, u32 src);
        u64 execOr64Impl(u64 dst, u64 src);

        u8 execXor8Impl(u8 dst, u8 src);
        u16 execXor16Impl(u16 dst, u16 src);
        u32 execXor32Impl(u32 dst, u32 src);
        u64 execXor64Impl(u64 dst, u64 src);

        template<typename Dst>
        void execCmpxchg32Impl(Dst dst, R32 src);

        template<typename Dst>
        void execCmpxchg64Impl(Dst dst, R64 src);

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
        void exec(const Adc<R32, SignExtended<u8>>&) override;
        void exec(const Adc<R32, M32>&) override;
        void exec(const Adc<M32, R32>&) override;
        void exec(const Adc<M32, Imm>&) override;

        void exec(const Sub<R32, R32>&) override;
        void exec(const Sub<R32, Imm>&) override;
        void exec(const Sub<R32, SignExtended<u8>>&) override;
        void exec(const Sub<R32, M32>&) override;
        void exec(const Sub<M32, R32>&) override;
        void exec(const Sub<M32, Imm>&) override;
        void exec(const Sub<R64, R64>&) override;
        void exec(const Sub<R64, Imm>&) override;
        void exec(const Sub<R64, SignExtended<u8>>&) override;
        void exec(const Sub<R64, M64>&) override;
        void exec(const Sub<M64, R64>&) override;
        void exec(const Sub<M64, Imm>&) override;

        void exec(const Sbb<R32, R32>&) override;
        void exec(const Sbb<R32, Imm>&) override;
        void exec(const Sbb<R32, SignExtended<u8>>&) override;
        void exec(const Sbb<R32, M32>&) override;
        void exec(const Sbb<M32, R32>&) override;
        void exec(const Sbb<M32, Imm>&) override;

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
        void exec(const And<R16, M16>&) override;
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

        void exec(const Not<R32>&) override;
        void exec(const Not<M32>&) override;
        void exec(const Not<R64>&) override;
        void exec(const Not<M64>&) override;

        void exec(const Xchg<R16, R16>&) override;
        void exec(const Xchg<R32, R32>&) override;
        void exec(const Xchg<M32, R32>&) override;

        void exec(const Xadd<R16, R16>&) override;
        void exec(const Xadd<R32, R32>&) override;
        void exec(const Xadd<M32, R32>&) override;

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

        void exec(const Push<SignExtended<u8>>&) override;
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
        void exec(const NotParsed&) override;
        void exec(const Unknown&) override;

        void exec(const Cdq&) override;
        void exec(const Cqo&) override;

        void exec(const Inc<R8>&) override;
        void exec(const Inc<M8>&) override;
        void exec(const Inc<M16>&) override;
        void exec(const Inc<R32>&) override;
        void exec(const Inc<M32>&) override;

        void exec(const Dec<R8>&) override;
        void exec(const Dec<M16>&) override;
        void exec(const Dec<R32>&) override;
        void exec(const Dec<M32>&) override;

        void exec(const Shr<R8, Imm>&) override;
        void exec(const Shr<R16, Imm>&) override;
        void exec(const Shr<R32, R8>&) override;
        void exec(const Shr<R32, Imm>&) override;
        void exec(const Shr<R64, R8>&) override;
        void exec(const Shr<R64, Imm>&) override;

        void exec(const Shl<R32, R8>&) override;
        void exec(const Shl<R32, Imm>&) override;
        void exec(const Shl<M32, Imm>&) override;
        void exec(const Shl<R64, R8>&) override;
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

        u16 execTzcnt16Impl(u16 src);
        u32 execTzcnt32Impl(u32 src);
        u64 execTzcnt64Impl(u64 src);

        void exec(const Tzcnt<R16, R16>&) override;
        void exec(const Tzcnt<R16, M16>&) override;
        void exec(const Tzcnt<R32, R32>&) override;
        void exec(const Tzcnt<R32, M32>&) override;
        void exec(const Tzcnt<R64, R64>&) override;
        void exec(const Tzcnt<R64, M64>&) override;

        void exec(const Test<R8, R8>&) override;
        void exec(const Test<R8, Imm>&) override;
        void exec(const Test<M8, R8>&) override;
        void exec(const Test<M8, Imm>&) override;
        void exec(const Test<R16, R16>&) override;
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
        void exec(const Bsf<R32, M32>&) override;

        void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&) override;
        void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&) override;
        
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

        void exec(const Movss<RSSE, M32>&) override;
        void exec(const Movss<M32, RSSE>&) override;

        void exec(const Movsd<RSSE, M64>&) override;
        void exec(const Movsd<M64, RSSE>&) override;

        u32 execAddssImpl(u32 dst, u32 src);
        u64 execAddsdImpl(u64 dst, u64 src);
        
        void exec(const Addss<RSSE, RSSE>&) override;
        void exec(const Addss<RSSE, M32>&) override;

        void exec(const Addsd<RSSE, RSSE>&) override;
        void exec(const Addsd<RSSE, M64>&) override;

        u64 execSubsdImpl(u64 dst, u64 src);

        void exec(const Subsd<RSSE, RSSE>&) override;
        void exec(const Subsd<RSSE, M64>&) override;

        u64 execMulsdImpl(u64 dst, u64 src);

        void exec(const Mulsd<RSSE, RSSE>&) override;
        void exec(const Mulsd<RSSE, M64>&) override;

        u64 execComisdImpl(u64 dst, u64 src);
        u64 execUcomisdImpl(u64 dst, u64 src);

        void exec(const Comisd<RSSE, RSSE>&) override;
        void exec(const Comisd<RSSE, M64>&) override;
        void exec(const Ucomisd<RSSE, RSSE>&) override;
        void exec(const Ucomisd<RSSE, M64>&) override;

        u64 execCvtsi2sd32Impl(u32 src);
        u64 execCvtsi2sd64Impl(u64 src);

        void exec(const Cvtsi2sd<RSSE, R32>&) override;
        void exec(const Cvtsi2sd<RSSE, M32>&) override;
        void exec(const Cvtsi2sd<RSSE, R64>&) override;
        void exec(const Cvtsi2sd<RSSE, M64>&) override;

        void exec(const Xorpd<RSSE, RSSE>&) override;

        void resolveFunctionName(const CallDirect& ins) override;

    };

}

#endif
