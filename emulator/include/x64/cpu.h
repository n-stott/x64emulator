#ifndef CPU_H
#define CPU_H

#include "x64/registers.h"
#include "x64/flags.h"
#include "x64/simd.h"
#include "x64/x87.h"
#include "x64/instructions/basicblock.h"
#include "x64/instructions/x64instruction.h"
#include <vector>

namespace x64 {

    class Mmu;
    class Cpu;

    class Cpu {
    public:
        explicit Cpu(Mmu& mmu);

        class Callback {
        public:
            virtual ~Callback() = default;
            virtual void onSyscall() = 0;
            virtual void onCall(u64 address) = 0;
            virtual void onRet() = 0;
        };

        void addCallback(Callback* callback);
        void removeCallback(Callback* callback);

        void exec(const X64Instruction&);

        BasicBlock createBasicBlock(const X64Instruction*, size_t) const;

        void exec(const BasicBlock&);
        
        void setSegmentBase(Segment segment, u64 base);
        u64 getSegmentBase(Segment segment) const;

        u8  get(R8 reg) const  { return regs_.get(reg); }
        u16 get(R16 reg) const { return regs_.get(reg); }
        u32 get(R32 reg) const { return regs_.get(reg); }
        u64 get(R64 reg) const { return regs_.get(reg); }
        u64 get(MMX reg) const { return regs_.get(reg); }
        Xmm get(XMM reg) const { return regs_.get(reg); }

        void set(R8 reg, u8 value) { regs_.set(reg, value); }
        void set(R16 reg, u16 value) { regs_.set(reg, value); }
        void set(R32 reg, u32 value) { regs_.set(reg, value); }
        void set(R64 reg, u64 value) { regs_.set(reg, value); }
        void set(MMX reg, u64 value) { regs_.set(reg, value); }
        void set(XMM reg, Xmm value) { regs_.set(reg, value); }

        template<Size size>
        U<size> xchg(R<size> reg, U<size> value) {
            U<size> regValue = regs_.get(reg);
            regs_.set(reg, value);
            return regValue;
        }

        u8  get(Ptr8 ptr) const;
        u16 get(Ptr16 ptr) const;
        u32 get(Ptr32 ptr) const;
        u64 get(Ptr64 ptr) const;
        f80 get(Ptr80 ptr) const;
        Xmm get(Ptr128 ptr) const;
        Xmm getUnaligned(Ptr128 ptr) const;

        void set(Ptr8 ptr, u8 value);
        void set(Ptr16 ptr, u16 value);
        void set(Ptr32 ptr, u32 value);
        void set(Ptr64 ptr, u64 value);
        void set(Ptr80 ptr, f80 value);
        void set(Ptr128 ptr, Xmm value);
        void setUnaligned(Ptr128 ptr, Xmm value);

        u8  xchg(Ptr8 ptr, u8 value);
        u16 xchg(Ptr16 ptr, u16 value);
        u32 xchg(Ptr32 ptr, u32 value);
        u64 xchg(Ptr64 ptr, u64 value);

        template<Size size>
        inline U<size> get(const RM<size>& rm) const {
            return rm.isReg ? get(rm.reg) : get(resolve(rm.mem));
        }
        
        template<Size size>
        inline void set(const RM<size>& rm, U<size> value) {
            return rm.isReg ? set(rm.reg, value) : set(resolve(rm.mem), value);
        }

        inline u64 get(const MMXM32& rm) const {
            return rm.isReg ? get(rm.reg) : get(resolve(rm.mem));
        }

        inline u64 get(const MMXM64& rm) const {
            return rm.isReg ? get(rm.reg) : get(resolve(rm.mem));
        }
        
        inline void set(const MMXM64& rm, u64 value) {
            return rm.isReg ? set(rm.reg, value) : set(resolve(rm.mem), value);
        }
        
        template<Size size>
        inline U<size> xchg(const RM<size>& rm, U<size> value) {
            return rm.isReg ? xchg<size>(rm.reg, value) : xchg(resolve(rm.mem), value);
        }

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        void push64(u64 value);
        u8 pop8();
        u16 pop16();
        u32 pop32();
        u64 pop64();

        struct State {
            Flags flags;
            Registers regs;
            X87Fpu x87fpu;
            SimdControlStatus mxcsr;
            std::array<u64, 8> segmentBase {{ 0, 0, 0, 0, 0, 0, 0, 0 }};
        };

        void save(State*) const;
        void load(const State&);

        
    private:
        friend class Jit;
        Mmu* mmu_;
        Flags flags_;
        Registers regs_;
        X87Fpu x87fpu_;
        SimdControlStatus mxcsr_;
        std::array<u64, 8> segmentBase_ {{ 0, 0, 0, 0, 0, 0, 0, 0 }};

        std::vector<Callback*> callbacks_;

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

        FPU_ROUNDING fpuRoundingMode() const;
        SIMD_ROUNDING simdRoundingMode() const;

        template<typename T>
        T  get(Imm value) const;

        u32 resolve(Encoding32 addr) const { return regs_.resolve(addr); }
        u64 resolve(Encoding64 addr) const { return regs_.resolve(addr); }

        template<Size size>
        SPtr<size> resolve(M<size> addr) const {
            return SPtr<size>{getSegmentBase(addr.segment) + resolve(addr.encoding)};
        }

        template<typename Dst>
        void execSet(Cond cond, Dst dst);

        template<typename Dst>
        void execCmpxchg8Impl(Dst dst, u8 src);

        template<typename Dst>
        void execCmpxchg16Impl(Dst dst, u16 src);

        template<typename Dst>
        void execCmpxchg32Impl(Dst dst, u32 src);

        template<typename Dst>
        void execCmpxchg64Impl(Dst dst, u64 src);

        void execLockCmpxchg8Impl(Ptr8 dst, u8 src);
        void execLockCmpxchg16Impl(Ptr16 dst, u16 src);
        void execLockCmpxchg32Impl(Ptr32 dst, u32 src);
        void execLockCmpxchg64Impl(Ptr64 dst, u64 src);

        static const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> execFunctions_;

    public:
        void execAddRM8RM8(const X64Instruction&);
        void execAddRM8Imm(const X64Instruction&);
        void execAddRM16RM16(const X64Instruction&);
        void execAddRM16Imm(const X64Instruction&);
        void execAddRM32RM32(const X64Instruction&);
        void execAddRM32Imm(const X64Instruction&);
        void execAddRM64RM64(const X64Instruction&);
        void execAddRM64Imm(const X64Instruction&);

        void execLockAddM8RM8(const X64Instruction&);
        void execLockAddM8Imm(const X64Instruction&);
        void execLockAddM16RM16(const X64Instruction&);
        void execLockAddM16Imm(const X64Instruction&);
        void execLockAddM32RM32(const X64Instruction&);
        void execLockAddM32Imm(const X64Instruction&);
        void execLockAddM64RM64(const X64Instruction&);
        void execLockAddM64Imm(const X64Instruction&);

        void execAdcRM8RM8(const X64Instruction&);
        void execAdcRM8Imm(const X64Instruction&);
        void execAdcRM16RM16(const X64Instruction&);
        void execAdcRM16Imm(const X64Instruction&);
        void execAdcRM32RM32(const X64Instruction&);
        void execAdcRM32Imm(const X64Instruction&);
        void execAdcRM64RM64(const X64Instruction&);
        void execAdcRM64Imm(const X64Instruction&);

        void execSubRM8RM8(const X64Instruction&);
        void execSubRM8Imm(const X64Instruction&);
        void execSubRM16RM16(const X64Instruction&);
        void execSubRM16Imm(const X64Instruction&);
        void execSubRM32RM32(const X64Instruction&);
        void execSubRM32Imm(const X64Instruction&);
        void execSubRM64RM64(const X64Instruction&);
        void execSubRM64Imm(const X64Instruction&);

        void execLockSubM8RM8(const X64Instruction&);
        void execLockSubM8Imm(const X64Instruction&);
        void execLockSubM16RM16(const X64Instruction&);
        void execLockSubM16Imm(const X64Instruction&);
        void execLockSubM32RM32(const X64Instruction&);
        void execLockSubM32Imm(const X64Instruction&);
        void execLockSubM64RM64(const X64Instruction&);
        void execLockSubM64Imm(const X64Instruction&);

        void execSbbRM8RM8(const X64Instruction&);
        void execSbbRM8Imm(const X64Instruction&);
        void execSbbRM16RM16(const X64Instruction&);
        void execSbbRM16Imm(const X64Instruction&);
        void execSbbRM32RM32(const X64Instruction&);
        void execSbbRM32Imm(const X64Instruction&);
        void execSbbRM64RM64(const X64Instruction&);
        void execSbbRM64Imm(const X64Instruction&);

        void execNegRM8(const X64Instruction&);
        void execNegRM16(const X64Instruction&);
        void execNegRM32(const X64Instruction&);
        void execNegRM64(const X64Instruction&);

        void execMulRM8(const X64Instruction&);
        void execMulRM16(const X64Instruction&);
        void execMulRM32(const X64Instruction&);
        void execMulRM64(const X64Instruction&);

        void execImul1RM16(const X64Instruction&);
        void execImul2R16RM16(const X64Instruction&);
        void execImul3R16RM16Imm(const X64Instruction&);
        void execImul1RM32(const X64Instruction&);
        void execImul2R32RM32(const X64Instruction&);
        void execImul3R32RM32Imm(const X64Instruction&);
        void execImul1RM64(const X64Instruction&);
        void execImul2R64RM64(const X64Instruction&);
        void execImul3R64RM64Imm(const X64Instruction&);

        void execDivRM8(const X64Instruction&);
        void execDivRM16(const X64Instruction&);
        void execDivRM32(const X64Instruction&);
        void execDivRM64(const X64Instruction&);

        void execIdivRM32(const X64Instruction&);
        void execIdivRM64(const X64Instruction&);

        void execAndRM8RM8(const X64Instruction&);
        void execAndRM8Imm(const X64Instruction&);
        void execAndRM16RM16(const X64Instruction&);
        void execAndRM16Imm(const X64Instruction&);
        void execAndRM32RM32(const X64Instruction&);
        void execAndRM32Imm(const X64Instruction&);
        void execAndRM64RM64(const X64Instruction&);
        void execAndRM64Imm(const X64Instruction&);

        void execOrRM8RM8(const X64Instruction&);
        void execOrRM8Imm(const X64Instruction&);
        void execOrRM16RM16(const X64Instruction&);
        void execOrRM16Imm(const X64Instruction&);
        void execOrRM32RM32(const X64Instruction&);
        void execOrRM32Imm(const X64Instruction&);
        void execOrRM64RM64(const X64Instruction&);
        void execOrRM64Imm(const X64Instruction&);

        void execLockOrM8RM8(const X64Instruction&);
        void execLockOrM8Imm(const X64Instruction&);
        void execLockOrM16RM16(const X64Instruction&);
        void execLockOrM16Imm(const X64Instruction&);
        void execLockOrM32RM32(const X64Instruction&);
        void execLockOrM32Imm(const X64Instruction&);
        void execLockOrM64RM64(const X64Instruction&);
        void execLockOrM64Imm(const X64Instruction&);

        void execXorRM8RM8(const X64Instruction&);
        void execXorRM8Imm(const X64Instruction&);
        void execXorRM16RM16(const X64Instruction&);
        void execXorRM16Imm(const X64Instruction&);
        void execXorRM32RM32(const X64Instruction&);
        void execXorRM32Imm(const X64Instruction&);
        void execXorRM64RM64(const X64Instruction&);
        void execXorRM64Imm(const X64Instruction&);

        void execNotRM8(const X64Instruction&);
        void execNotRM16(const X64Instruction&);
        void execNotRM32(const X64Instruction&);
        void execNotRM64(const X64Instruction&);

        void execXchgRM8R8(const X64Instruction&);
        void execXchgRM16R16(const X64Instruction&);
        void execXchgRM32R32(const X64Instruction&);
        void execXchgRM64R64(const X64Instruction&);

        void execXaddRM16R16(const X64Instruction&);
        void execXaddRM32R32(const X64Instruction&);
        void execXaddRM64R64(const X64Instruction&);

        void execLockXaddM16R16(const X64Instruction&);
        void execLockXaddM32R32(const X64Instruction&);
        void execLockXaddM64R64(const X64Instruction&);

        template<Size size>
        void execMovRR(const X64Instruction&);

        void execMovMMXMMX(const X64Instruction&);

        template<Size size>
        void execMovRM(const X64Instruction&);

        template<Size size>
        void execMovMR(const X64Instruction&);

        template<Size size>
        void execMovRImm(const X64Instruction&);

        template<Size size>
        void execMovMImm(const X64Instruction&);

        void execMovq2dq(const X64Instruction&);
        void execMovdq2q(const X64Instruction&);

        void execMovaXMMM128(const X64Instruction&);
        void execMovaM128XMM(const X64Instruction&);
        void execMovuXMMM128(const X64Instruction&);
        void execMovuM128XMM(const X64Instruction&);

        void execMovsxR16RM8(const X64Instruction&);
        void execMovsxR32RM8(const X64Instruction&);
        void execMovsxR32RM16(const X64Instruction&);
        void execMovsxR64RM8(const X64Instruction&);
        void execMovsxR64RM16(const X64Instruction&);
        void execMovsxR64RM32(const X64Instruction&);

        void execMovzxR16RM8(const X64Instruction&);
        void execMovzxR32RM8(const X64Instruction&);
        void execMovzxR32RM16(const X64Instruction&);
        void execMovzxR64RM8(const X64Instruction&);
        void execMovzxR64RM16(const X64Instruction&);
        void execMovzxR64RM32(const X64Instruction&);

        void execLeaR32Encoding32(const X64Instruction&);
        void execLeaR64Encoding32(const X64Instruction&);
        void execLeaR32Encoding64(const X64Instruction&);
        void execLeaR64Encoding64(const X64Instruction&);

        void execPushImm(const X64Instruction&);
        void execPushRM32(const X64Instruction&);
        void execPushRM64(const X64Instruction&);

        void execPopR32(const X64Instruction&);
        void execPopR64(const X64Instruction&);
        void execPopM32(const X64Instruction&);
        void execPopM64(const X64Instruction&);

        void execPushfq(const X64Instruction&);
        void execPopfq(const X64Instruction&);

        void execCallDirect(const X64Instruction&);
        void execCallIndirectRM32(const X64Instruction&);
        void execCallIndirectRM64(const X64Instruction&);
        void execRet(const X64Instruction&);
        void execRetImm(const X64Instruction&);

        void execLeave(const X64Instruction&);
        void execHalt(const X64Instruction&);
        void execNop(const X64Instruction&);
        void execUd2(const X64Instruction&);
        void execSyscall(const X64Instruction&);
        void execUnknown(const X64Instruction&);

        void execCdq(const X64Instruction&);
        void execCqo(const X64Instruction&);

        void execIncRM8(const X64Instruction&);
        void execIncRM16(const X64Instruction&);
        void execIncRM32(const X64Instruction&);
        void execIncRM64(const X64Instruction&);

        void execLockIncM8(const X64Instruction&);
        void execLockIncM16(const X64Instruction&);
        void execLockIncM32(const X64Instruction&);
        void execLockIncM64(const X64Instruction&);

        void execDecRM8(const X64Instruction&);
        void execDecRM16(const X64Instruction&);
        void execDecRM32(const X64Instruction&);
        void execDecRM64(const X64Instruction&);

        void execLockDecM8(const X64Instruction&);
        void execLockDecM16(const X64Instruction&);
        void execLockDecM32(const X64Instruction&);
        void execLockDecM64(const X64Instruction&);

        void execShrRM8R8(const X64Instruction&);
        void execShrRM8Imm(const X64Instruction&);
        void execShrRM16R8(const X64Instruction&);
        void execShrRM16Imm(const X64Instruction&);
        void execShrRM32R8(const X64Instruction&);
        void execShrRM32Imm(const X64Instruction&);
        void execShrRM64R8(const X64Instruction&);
        void execShrRM64Imm(const X64Instruction&);

        void execShlRM8R8(const X64Instruction&);
        void execShlRM8Imm(const X64Instruction&);
        void execShlRM16R8(const X64Instruction&);
        void execShlRM16Imm(const X64Instruction&);
        void execShlRM32R8(const X64Instruction&);
        void execShlRM32Imm(const X64Instruction&);
        void execShlRM64R8(const X64Instruction&);
        void execShlRM64Imm(const X64Instruction&);

        void execShldRM32R32R8(const X64Instruction&);
        void execShldRM32R32Imm(const X64Instruction&);
        void execShldRM64R64R8(const X64Instruction&);
        void execShldRM64R64Imm(const X64Instruction&);

        void execShrdRM32R32R8(const X64Instruction&);
        void execShrdRM32R32Imm(const X64Instruction&);
        void execShrdRM64R64R8(const X64Instruction&);
        void execShrdRM64R64Imm(const X64Instruction&);

        void execSarRM8R8(const X64Instruction&);
        void execSarRM8Imm(const X64Instruction&);
        void execSarRM16R8(const X64Instruction&);
        void execSarRM16Imm(const X64Instruction&);
        void execSarRM32R8(const X64Instruction&);
        void execSarRM32Imm(const X64Instruction&);
        void execSarRM64R8(const X64Instruction&);
        void execSarRM64Imm(const X64Instruction&);

        void execSarxR32RM32R32(const X64Instruction&);
        void execSarxR64RM64R64(const X64Instruction&);
        void execShlxR32RM32R32(const X64Instruction&);
        void execShlxR64RM64R64(const X64Instruction&);
        void execShrxR32RM32R32(const X64Instruction&);
        void execShrxR64RM64R64(const X64Instruction&);

        void execRclRM8R8(const X64Instruction&);
        void execRclRM8Imm(const X64Instruction&);
        void execRclRM16R8(const X64Instruction&);
        void execRclRM16Imm(const X64Instruction&);
        void execRclRM32R8(const X64Instruction&);
        void execRclRM32Imm(const X64Instruction&);
        void execRclRM64R8(const X64Instruction&);
        void execRclRM64Imm(const X64Instruction&);

        void execRcrRM8R8(const X64Instruction&);
        void execRcrRM8Imm(const X64Instruction&);
        void execRcrRM16R8(const X64Instruction&);
        void execRcrRM16Imm(const X64Instruction&);
        void execRcrRM32R8(const X64Instruction&);
        void execRcrRM32Imm(const X64Instruction&);
        void execRcrRM64R8(const X64Instruction&);
        void execRcrRM64Imm(const X64Instruction&);

        void execRolRM8R8(const X64Instruction&);
        void execRolRM8Imm(const X64Instruction&);
        void execRolRM16R8(const X64Instruction&);
        void execRolRM16Imm(const X64Instruction&);
        void execRolRM32R8(const X64Instruction&);
        void execRolRM32Imm(const X64Instruction&);
        void execRolRM64R8(const X64Instruction&);
        void execRolRM64Imm(const X64Instruction&);

        void execRorRM8R8(const X64Instruction&);
        void execRorRM8Imm(const X64Instruction&);
        void execRorRM16R8(const X64Instruction&);
        void execRorRM16Imm(const X64Instruction&);
        void execRorRM32R8(const X64Instruction&);
        void execRorRM32Imm(const X64Instruction&);
        void execRorRM64R8(const X64Instruction&);
        void execRorRM64Imm(const X64Instruction&);

        void execTzcntR16RM16(const X64Instruction&);
        void execTzcntR32RM32(const X64Instruction&);
        void execTzcntR64RM64(const X64Instruction&);

        void execBtRM16R16(const X64Instruction&);
        void execBtRM16Imm(const X64Instruction&);
        void execBtRM32R32(const X64Instruction&);
        void execBtRM32Imm(const X64Instruction&);
        void execBtRM64R64(const X64Instruction&);
        void execBtRM64Imm(const X64Instruction&);

        void execBtrRM16R16(const X64Instruction&);
        void execBtrRM16Imm(const X64Instruction&);
        void execBtrRM32R32(const X64Instruction&);
        void execBtrRM32Imm(const X64Instruction&);
        void execBtrRM64R64(const X64Instruction&);
        void execBtrRM64Imm(const X64Instruction&);

        void execBtcRM16R16(const X64Instruction&);
        void execBtcRM16Imm(const X64Instruction&);
        void execBtcRM32R32(const X64Instruction&);
        void execBtcRM32Imm(const X64Instruction&);
        void execBtcRM64R64(const X64Instruction&);
        void execBtcRM64Imm(const X64Instruction&);

        void execBtsRM16R16(const X64Instruction&);
        void execBtsRM16Imm(const X64Instruction&);
        void execBtsRM32R32(const X64Instruction&);
        void execBtsRM32Imm(const X64Instruction&);
        void execBtsRM64R64(const X64Instruction&);
        void execBtsRM64Imm(const X64Instruction&);

        void execLockBtsM16R16(const X64Instruction&);
        void execLockBtsM16Imm(const X64Instruction&);
        void execLockBtsM32R32(const X64Instruction&);
        void execLockBtsM32Imm(const X64Instruction&);
        void execLockBtsM64R64(const X64Instruction&);
        void execLockBtsM64Imm(const X64Instruction&);

        void execTestRM8R8(const X64Instruction&);
        void execTestRM8Imm(const X64Instruction&);
        void execTestRM16R16(const X64Instruction&);
        void execTestRM16Imm(const X64Instruction&);
        void execTestRM32R32(const X64Instruction&);
        void execTestRM32Imm(const X64Instruction&);
        void execTestRM64R64(const X64Instruction&);
        void execTestRM64Imm(const X64Instruction&);

        void execCmpRM8RM8(const X64Instruction&);
        void execCmpRM8Imm(const X64Instruction&);
        void execCmpRM16RM16(const X64Instruction&);
        void execCmpRM16Imm(const X64Instruction&);
        void execCmpRM32RM32(const X64Instruction&);
        void execCmpRM32Imm(const X64Instruction&);
        void execCmpRM64RM64(const X64Instruction&);
        void execCmpRM64Imm(const X64Instruction&);

        void execCmpxchgRM8R8(const X64Instruction&);
        void execCmpxchgRM16R16(const X64Instruction&);
        void execCmpxchgRM32R32(const X64Instruction&);
        void execCmpxchgRM64R64(const X64Instruction&);
        void execCmpxchg16BM128(const X64Instruction&);

        void execLockCmpxchgM8R8(const X64Instruction&);
        void execLockCmpxchgM16R16(const X64Instruction&);
        void execLockCmpxchgM32R32(const X64Instruction&);
        void execLockCmpxchgM64R64(const X64Instruction&);
        void execLockCmpxchg16BM128(const X64Instruction&);

        void execSetRM8(const X64Instruction&);

        void execJmpRM32(const X64Instruction&);
        void execJmpRM64(const X64Instruction&);
        void execJmpu32(const X64Instruction&);
        void execJe(const X64Instruction&);
        void execJne(const X64Instruction&);
        void execJcc(const X64Instruction&);
        void execJrcxz(const X64Instruction&);

        void execBsrR16R16(const X64Instruction&);
        void execBsrR16M16(const X64Instruction&);
        void execBsrR32R32(const X64Instruction&);
        void execBsrR32M32(const X64Instruction&);
        void execBsrR64R64(const X64Instruction&);
        void execBsrR64M64(const X64Instruction&);
        
        void execBsfR16R16(const X64Instruction&);
        void execBsfR16M16(const X64Instruction&);
        void execBsfR32R32(const X64Instruction&);
        void execBsfR32M32(const X64Instruction&);
        void execBsfR64R64(const X64Instruction&);
        void execBsfR64M64(const X64Instruction&);

        void execCld(const X64Instruction&);
        void execStd(const X64Instruction&);

        void execMovsM8M8(const X64Instruction&);
        void execMovsM16M16(const X64Instruction&);
        void execMovsM64M64(const X64Instruction&);
        void execRepMovsM8M8(const X64Instruction&);
        void execRepMovsM16M16(const X64Instruction&);
        void execRepMovsM32M32(const X64Instruction&);
        void execRepMovsM64M64(const X64Instruction&);
        
        void execRepCmpsM8M8(const X64Instruction&);
        
        void execRepStosM8R8(const X64Instruction&);
        void execRepStosM16R16(const X64Instruction&);
        void execRepStosM32R32(const X64Instruction&);
        void execRepStosM64R64(const X64Instruction&);

        void execRepNZScasR8M8(const X64Instruction&);
        void execRepNZScasR16M16(const X64Instruction&);
        void execRepNZScasR32M32(const X64Instruction&);
        void execRepNZScasR64M64(const X64Instruction&);

        void execCmovR16RM16(const X64Instruction&);
        void execCmovR32RM32(const X64Instruction&);
        void execCmovR64RM64(const X64Instruction&);

        void execCwde(const X64Instruction&);
        void execCdqe(const X64Instruction&);

        void execBswapR32(const X64Instruction&);
        void execBswapR64(const X64Instruction&);

        void execPopcntR16RM16(const X64Instruction&);
        void execPopcntR32RM32(const X64Instruction&);
        void execPopcntR64RM64(const X64Instruction&);

        void execMovapsXMMM128XMMM128(const X64Instruction&);

        void execMovdMMXRM32(const X64Instruction&);
        void execMovdRM32MMX(const X64Instruction&);
        void execMovdMMXRM64(const X64Instruction&);
        void execMovdRM64MMX(const X64Instruction&);

        void execMovdXMMRM32(const X64Instruction&);
        void execMovdRM32XMM(const X64Instruction&);
        void execMovdXMMRM64(const X64Instruction&);
        void execMovdRM64XMM(const X64Instruction&);

        void execMovqMMXRM64(const X64Instruction&);
        void execMovqRM64MMX(const X64Instruction&);
        void execMovqXMMRM64(const X64Instruction&);
        void execMovqRM64XMM(const X64Instruction&);

        void execFldz(const X64Instruction&);
        void execFld1(const X64Instruction&);
        void execFldST(const X64Instruction&);
        void execFldM32(const X64Instruction&);
        void execFldM64(const X64Instruction&);
        void execFldM80(const X64Instruction&);
        void execFildM16(const X64Instruction&);
        void execFildM32(const X64Instruction&);
        void execFildM64(const X64Instruction&);
        void execFstpST(const X64Instruction&);
        void execFstpM32(const X64Instruction&);
        void execFstpM64(const X64Instruction&);
        void execFstpM80(const X64Instruction&);
        void execFistpM16(const X64Instruction&);
        void execFistpM32(const X64Instruction&);
        void execFistpM64(const X64Instruction&);
        void execFxchST(const X64Instruction&);

        void execFaddM32(const X64Instruction&);
        void execFaddM64(const X64Instruction&);
        void execFaddpST(const X64Instruction&);
        void execFsubpST(const X64Instruction&);
        void execFsubrpST(const X64Instruction&);
        void execFmul1M32(const X64Instruction&);
        void execFmul1M64(const X64Instruction&);
        void execFdivSTST(const X64Instruction&);
        void execFdivM32(const X64Instruction&);
        void execFdivpSTST(const X64Instruction&);
        void execFdivrSTST(const X64Instruction&);
        void execFdivrM32(const X64Instruction&);
        void execFdivrpSTST(const X64Instruction&);

        void execFcomiSTST(const X64Instruction&);
        void execFucomiSTST(const X64Instruction&);
        void execFrndint(const X64Instruction&);

        void execFcmovST(const X64Instruction&);

        void execFnstcwM16(const X64Instruction&);
        void execFldcwM16(const X64Instruction&);

        void execFnstswR16(const X64Instruction&);
        void execFnstswM16(const X64Instruction&);

        void execFnstenvM224(const X64Instruction&);
        void execFldenvM224(const X64Instruction&);

        void execEmms(const X64Instruction&);

        void execMovssXMMM32(const X64Instruction&);
        void execMovssM32XMM(const X64Instruction&);
        void execMovssXMMXMM(const X64Instruction&);

        void execMovsdXMMM64(const X64Instruction&);
        void execMovsdM64XMM(const X64Instruction&);
        void execMovsdXMMXMM(const X64Instruction&);

        void execAddpsXMMXMMM128(const X64Instruction&);
        void execAddpdXMMXMMM128(const X64Instruction&);
        void execAddssXMMXMM(const X64Instruction&);
        void execAddssXMMM32(const X64Instruction&);
        void execAddsdXMMXMM(const X64Instruction&);
        void execAddsdXMMM64(const X64Instruction&);

        void execSubpsXMMXMMM128(const X64Instruction&);
        void execSubpdXMMXMMM128(const X64Instruction&);
        void execSubssXMMXMM(const X64Instruction&);
        void execSubssXMMM32(const X64Instruction&);
        void execSubsdXMMXMM(const X64Instruction&);
        void execSubsdXMMM64(const X64Instruction&);

        void execMulpsXMMXMMM128(const X64Instruction&);
        void execMulpdXMMXMMM128(const X64Instruction&);
        void execMulssXMMXMM(const X64Instruction&);
        void execMulssXMMM32(const X64Instruction&);
        void execMulsdXMMXMM(const X64Instruction&);
        void execMulsdXMMM64(const X64Instruction&);

        void execDivpsXMMXMMM128(const X64Instruction&);
        void execDivpdXMMXMMM128(const X64Instruction&);
        void execDivssXMMXMM(const X64Instruction&);
        void execDivssXMMM32(const X64Instruction&);
        void execDivsdXMMXMM(const X64Instruction&);
        void execDivsdXMMM64(const X64Instruction&);

        void execSqrtpsXMMXMMM128(const X64Instruction&);
        void execSqrtpdXMMXMMM128(const X64Instruction&);
        void execSqrtssXMMXMM(const X64Instruction&);
        void execSqrtssXMMM32(const X64Instruction&);
        void execSqrtsdXMMXMM(const X64Instruction&);
        void execSqrtsdXMMM64(const X64Instruction&);

        void execComissXMMXMM(const X64Instruction&);
        void execComissXMMM32(const X64Instruction&);
        void execComisdXMMXMM(const X64Instruction&);
        void execComisdXMMM64(const X64Instruction&);
        void execUcomissXMMXMM(const X64Instruction&);
        void execUcomissXMMM32(const X64Instruction&);
        void execUcomisdXMMXMM(const X64Instruction&);
        void execUcomisdXMMM64(const X64Instruction&);

        void execMaxssXMMXMM(const X64Instruction&);
        void execMaxssXMMM32(const X64Instruction&);
        void execMaxsdXMMXMM(const X64Instruction&);
        void execMaxsdXMMM64(const X64Instruction&);

        void execMinssXMMXMM(const X64Instruction&);
        void execMinssXMMM32(const X64Instruction&);
        void execMinsdXMMXMM(const X64Instruction&);
        void execMinsdXMMM64(const X64Instruction&);

        void execMaxpsXMMXMMM128(const X64Instruction&);
        void execMaxpdXMMXMMM128(const X64Instruction&);

        void execMinpsXMMXMMM128(const X64Instruction&);
        void execMinpdXMMXMMM128(const X64Instruction&);

        void execCmpssXMMXMM(const X64Instruction&);
        void execCmpssXMMM32(const X64Instruction&);
        void execCmpsdXMMXMM(const X64Instruction&);
        void execCmpsdXMMM64(const X64Instruction&);
        void execCmppsXMMXMMM128(const X64Instruction&);
        void execCmppdXMMXMMM128(const X64Instruction&);

        void execCvtsi2ssXMMRM32(const X64Instruction&);
        void execCvtsi2ssXMMRM64(const X64Instruction&);
        void execCvtsi2sdXMMRM32(const X64Instruction&);
        void execCvtsi2sdXMMRM64(const X64Instruction&);

        void execCvtss2sdXMMXMM(const X64Instruction&);
        void execCvtss2sdXMMM32(const X64Instruction&);

        void execCvtss2siR32XMM(const X64Instruction&);
        void execCvtss2siR32M32(const X64Instruction&);
        void execCvtss2siR64XMM(const X64Instruction&);
        void execCvtss2siR64M32(const X64Instruction&);

        void execCvtsd2siR32XMM(const X64Instruction&);
        void execCvtsd2siR32M64(const X64Instruction&);
        void execCvtsd2siR64XMM(const X64Instruction&);
        void execCvtsd2siR64M64(const X64Instruction&);

        void execCvtsd2ssXMMXMM(const X64Instruction&);
        void execCvtsd2ssXMMM64(const X64Instruction&);

        void execCvttps2dqXMMXMMM128(const X64Instruction&);

        void execCvttss2siR32XMM(const X64Instruction&);
        void execCvttss2siR32M32(const X64Instruction&);
        void execCvttss2siR64XMM(const X64Instruction&);
        void execCvttss2siR64M32(const X64Instruction&);

        void execCvttsd2siR32XMM(const X64Instruction&);
        void execCvttsd2siR32M64(const X64Instruction&);
        void execCvttsd2siR64XMM(const X64Instruction&);
        void execCvttsd2siR64M64(const X64Instruction&);

        void execCvtdq2psXMMXMMM128(const X64Instruction&);
        void execCvtdq2pdXMMXMM(const X64Instruction&);
        void execCvtdq2pdXMMM64(const X64Instruction&);

        void execCvtps2dqXMMXMMM128(const X64Instruction&);

        void execCvtpd2psXMMXMMM128(const X64Instruction&);

        void execStmxcsrM32(const X64Instruction&);
        void execLdmxcsrM32(const X64Instruction&);

        void execPandMMXMMXM64(const X64Instruction&);
        void execPandnMMXMMXM64(const X64Instruction&);
        void execPorMMXMMXM64(const X64Instruction&);
        void execPxorMMXMMXM64(const X64Instruction&);

        void execPandXMMXMMM128(const X64Instruction&);
        void execPandnXMMXMMM128(const X64Instruction&);
        void execPorXMMXMMM128(const X64Instruction&);
        void execPxorXMMXMMM128(const X64Instruction&);

        void execAndpdXMMXMMM128(const X64Instruction&);
        void execAndnpdXMMXMMM128(const X64Instruction&);
        void execOrpdXMMXMMM128(const X64Instruction&);
        void execXorpdXMMXMMM128(const X64Instruction&);

        void execShufpsXMMXMMM128Imm(const X64Instruction&);
        void execShufpdXMMXMMM128Imm(const X64Instruction&);

        void execMovlpsXMMM64(const X64Instruction&);
        void execMovlpsM64XMM(const X64Instruction&);
        void execMovhpsXMMM64(const X64Instruction&);
        void execMovhpsM64XMM(const X64Instruction&);
        void execMovhlpsXMMXMM(const X64Instruction&);
        void execMovlhpsXMMXMM(const X64Instruction&);

        void execPinsrwMMXR32Imm(const X64Instruction&);
        void execPinsrwMMXM16Imm(const X64Instruction&);

        void execPinsrwXMMR32Imm(const X64Instruction&);
        void execPinsrwXMMM16Imm(const X64Instruction&);
        void execPextrwR32XMMImm(const X64Instruction&);
        void execPextrwM16XMMImm(const X64Instruction&);

        void execPunpcklbwMMXMMXM32(const X64Instruction&);
        void execPunpcklwdMMXMMXM32(const X64Instruction&);
        void execPunpckldqMMXMMXM32(const X64Instruction&);
        void execPunpcklbwXMMXMMM128(const X64Instruction&);
        void execPunpcklwdXMMXMMM128(const X64Instruction&);
        void execPunpckldqXMMXMMM128(const X64Instruction&);
        void execPunpcklqdqXMMXMMM128(const X64Instruction&);

        void execPunpckhbwMMXMMXM64(const X64Instruction&);
        void execPunpckhwdMMXMMXM64(const X64Instruction&);
        void execPunpckhdqMMXMMXM64(const X64Instruction&);
        void execPunpckhbwXMMXMMM128(const X64Instruction&);
        void execPunpckhwdXMMXMMM128(const X64Instruction&);
        void execPunpckhdqXMMXMMM128(const X64Instruction&);
        void execPunpckhqdqXMMXMMM128(const X64Instruction&);

        void execPshufbMMXMMXM64(const X64Instruction&);
        void execPshufbXMMXMMM128(const X64Instruction&);
        void execPshufwMMXMMXM64Imm(const X64Instruction&);
        void execPshuflwXMMXMMM128Imm(const X64Instruction&);
        void execPshufhwXMMXMMM128Imm(const X64Instruction&);
        void execPshufdXMMXMMM128Imm(const X64Instruction&);

        void execPcmpeqbMMXMMXM64(const X64Instruction&);
        void execPcmpeqwMMXMMXM64(const X64Instruction&);
        void execPcmpeqdMMXMMXM64(const X64Instruction&);

        void execPcmpeqbXMMXMMM128(const X64Instruction&);
        void execPcmpeqwXMMXMMM128(const X64Instruction&);
        void execPcmpeqdXMMXMMM128(const X64Instruction&);
        void execPcmpeqqXMMXMMM128(const X64Instruction&);

        void execPcmpgtbMMXMMXM64(const X64Instruction&);
        void execPcmpgtwMMXMMXM64(const X64Instruction&);
        void execPcmpgtdMMXMMXM64(const X64Instruction&);

        void execPcmpgtbXMMXMMM128(const X64Instruction&);
        void execPcmpgtwXMMXMMM128(const X64Instruction&);
        void execPcmpgtdXMMXMMM128(const X64Instruction&);
        void execPcmpgtqXMMXMMM128(const X64Instruction&);

        void execPmovmskbR32MMX(const X64Instruction&);
        void execPmovmskbR64MMX(const X64Instruction&);
        void execPmovmskbR32XMM(const X64Instruction&);
        void execPmovmskbR64XMM(const X64Instruction&);

        void execPaddbMMXMMXM64(const X64Instruction&);
        void execPaddwMMXMMXM64(const X64Instruction&);
        void execPadddMMXMMXM64(const X64Instruction&);
        void execPaddqMMXMMXM64(const X64Instruction&);
        void execPaddsbMMXMMXM64(const X64Instruction&);
        void execPaddswMMXMMXM64(const X64Instruction&);
        void execPaddusbMMXMMXM64(const X64Instruction&);
        void execPadduswMMXMMXM64(const X64Instruction&);

        void execPaddbXMMXMMM128(const X64Instruction&);
        void execPaddwXMMXMMM128(const X64Instruction&);
        void execPadddXMMXMMM128(const X64Instruction&);
        void execPaddqXMMXMMM128(const X64Instruction&);
        void execPaddsbXMMXMMM128(const X64Instruction&);
        void execPaddswXMMXMMM128(const X64Instruction&);
        void execPaddusbXMMXMMM128(const X64Instruction&);
        void execPadduswXMMXMMM128(const X64Instruction&);

        void execPsubbMMXMMXM64(const X64Instruction&);
        void execPsubwMMXMMXM64(const X64Instruction&);
        void execPsubdMMXMMXM64(const X64Instruction&);
        void execPsubqMMXMMXM64(const X64Instruction&);
        void execPsubsbMMXMMXM64(const X64Instruction&);
        void execPsubswMMXMMXM64(const X64Instruction&);
        void execPsubusbMMXMMXM64(const X64Instruction&);
        void execPsubuswMMXMMXM64(const X64Instruction&);

        void execPsubbXMMXMMM128(const X64Instruction&);
        void execPsubwXMMXMMM128(const X64Instruction&);
        void execPsubdXMMXMMM128(const X64Instruction&);
        void execPsubqXMMXMMM128(const X64Instruction&);
        void execPsubsbXMMXMMM128(const X64Instruction&);
        void execPsubswXMMXMMM128(const X64Instruction&);
        void execPsubusbXMMXMMM128(const X64Instruction&);
        void execPsubuswXMMXMMM128(const X64Instruction&);

        void execPmulhuwMMXMMXM64(const X64Instruction&);
        void execPmulhwMMXMMXM64(const X64Instruction&);
        void execPmullwMMXMMXM64(const X64Instruction&);
        void execPmuludqMMXMMXM64(const X64Instruction&);

        void execPmulhuwXMMXMMM128(const X64Instruction&);
        void execPmulhwXMMXMMM128(const X64Instruction&);
        void execPmullwXMMXMMM128(const X64Instruction&);
        void execPmuludqXMMXMMM128(const X64Instruction&);

        void execPmaddwdMMXMMXM64(const X64Instruction&);
        void execPmaddwdXMMXMMM128(const X64Instruction&);

        void execPsadbwMMXMMXM64(const X64Instruction&);
        void execPsadbwXMMXMMM128(const X64Instruction&);

        void execPavgbMMXMMXM64(const X64Instruction&);
        void execPavgwMMXMMXM64(const X64Instruction&);
        void execPavgbXMMXMMM128(const X64Instruction&);
        void execPavgwXMMXMMM128(const X64Instruction&);

        void execPmaxswMMXMMXM64(const X64Instruction&);
        void execPmaxswXMMXMMM128(const X64Instruction&);
        void execPmaxubMMXMMXM64(const X64Instruction&);
        void execPmaxubXMMXMMM128(const X64Instruction&);

        void execPminswMMXMMXM64(const X64Instruction&);
        void execPminswXMMXMMM128(const X64Instruction&);
        void execPminubMMXMMXM64(const X64Instruction&);
        void execPminubXMMXMMM128(const X64Instruction&);

        void execPtestXMMXMMM128(const X64Instruction&);

        void execPsrawMMXImm(const X64Instruction&);
        void execPsrawMMXMMXM64(const X64Instruction&);
        void execPsradMMXImm(const X64Instruction&);
        void execPsradMMXMMXM64(const X64Instruction&);

        void execPsrawXMMImm(const X64Instruction&);
        void execPsrawXMMXMMM128(const X64Instruction&);
        void execPsradXMMImm(const X64Instruction&);
        void execPsradXMMXMMM128(const X64Instruction&);

        void execPsllwMMXImm(const X64Instruction&);
        void execPsllwMMXMMXM64(const X64Instruction&);
        void execPslldMMXImm(const X64Instruction&);
        void execPslldMMXMMXM64(const X64Instruction&);
        void execPsllqMMXImm(const X64Instruction&);
        void execPsllqMMXMMXM64(const X64Instruction&);
        void execPsrlwMMXImm(const X64Instruction&);
        void execPsrlwMMXMMXM64(const X64Instruction&);
        void execPsrldMMXImm(const X64Instruction&);
        void execPsrldMMXMMXM64(const X64Instruction&);
        void execPsrlqMMXImm(const X64Instruction&);
        void execPsrlqMMXMMXM64(const X64Instruction&);

        void execPsllwXMMImm(const X64Instruction&);
        void execPsllwXMMXMMM128(const X64Instruction&);
        void execPslldXMMImm(const X64Instruction&);
        void execPslldXMMXMMM128(const X64Instruction&);
        void execPsllqXMMImm(const X64Instruction&);
        void execPsllqXMMXMMM128(const X64Instruction&);
        void execPsrlwXMMImm(const X64Instruction&);
        void execPsrlwXMMXMMM128(const X64Instruction&);
        void execPsrldXMMImm(const X64Instruction&);
        void execPsrldXMMXMMM128(const X64Instruction&);
        void execPsrlqXMMImm(const X64Instruction&);
        void execPsrlqXMMXMMM128(const X64Instruction&);

        void execPslldqXMMImm(const X64Instruction&);
        void execPsrldqXMMImm(const X64Instruction&);
        
        void execPcmpistriXMMXMMM128Imm(const X64Instruction&);

        void execPackuswbMMXMMXM64(const X64Instruction&);
        void execPacksswbMMXMMXM64(const X64Instruction&);
        void execPackssdwMMXMMXM64(const X64Instruction&);

        void execPackuswbXMMXMMM128(const X64Instruction&);
        void execPackusdwXMMXMMM128(const X64Instruction&);
        void execPacksswbXMMXMMM128(const X64Instruction&);
        void execPackssdwXMMXMMM128(const X64Instruction&);

        void execUnpckhpsXMMXMMM128(const X64Instruction&);
        void execUnpckhpdXMMXMMM128(const X64Instruction&);
        void execUnpcklpsXMMXMMM128(const X64Instruction&);
        void execUnpcklpdXMMXMMM128(const X64Instruction&);

        void execMovmskpsR32XMM(const X64Instruction&);
        void execMovmskpsR64XMM(const X64Instruction&);
        void execMovmskpdR32XMM(const X64Instruction&);
        void execMovmskpdR64XMM(const X64Instruction&);

        void execLddquXMMM128(const X64Instruction&);
        void execMovshdupXMMXMMM128(const X64Instruction&);
        void execMovddupXMMXMM(const X64Instruction&);
        void execMovddupXMMM64(const X64Instruction&);

        void execPalignrMMXMMXM64Imm(const X64Instruction&);
        void execPalignrXMMXMMM128Imm(const X64Instruction&);

        void execPhaddwMMXMMXM64(const X64Instruction&);
        void execPhaddwXMXXMXM128(const X64Instruction&);
        void execPhadddMMXMMXM64(const X64Instruction&);
        void execPhadddXMXXMXM128(const X64Instruction&);
        void execPmaddubswMMXMMXM64(const X64Instruction&);
        void execPmaddubswXMMXMMM128(const X64Instruction&);

        void execPmulhrswMMXMMXM64(const X64Instruction&);
        void execPmulhrswXMMXMMM128(const X64Instruction&);
        
        void execRdtsc(const X64Instruction&);

        void execCpuid(const X64Instruction&);
        void execXgetbv(const X64Instruction&);

        void execFxsaveM4096(const X64Instruction&);
        void execFxrstorM4096(const X64Instruction&);

        void execFwait(const X64Instruction&);

        void execRdpkru(const X64Instruction&);
        void execWrpkru(const X64Instruction&);

        void execRdsspd(const X64Instruction&);

        void execPause(const X64Instruction&);

        void execUnimplemented(const X64Instruction&);

    };

}

#endif
