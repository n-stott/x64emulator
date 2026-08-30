#ifndef X64CPU_H
#define X64CPU_H

#include "arch/x64/registers.h"
#include "arch/x64/flags.h"
#include "arch/x64/simd.h"
#include "arch/x64/x87.h"
#include "arch/x64/instructions/basicblock.h"
#include "arch/x64/instructions/instruction.h"
#include "smallvector.h"
#include <vector>

namespace mem {
    class Mmu;
}

namespace x64 {

    class Cpu;

    class Cpu {
    public:
        explicit Cpu(mem::Mmu& mmu);

        class Callback {
        public:
            virtual ~Callback() = default;
            virtual void onSyscall() = 0;
            virtual void onCall(u64 address) = 0;
            virtual void onRet() = 0;
            virtual void onStackChange(u64 stackptr) = 0;
        };

        void addCallback(Callback* callback);
        void removeCallback(Callback* callback);

        void exec(const Instruction&);

        static BasicBlock createBasicBlock(const Instruction*, size_t);

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

        template<mem::Size size>
        U<size> xchg(R<size> reg, U<size> value) {
            U<size> regValue = regs_.get(reg);
            regs_.set(reg, value);
            return regValue;
        }

        u8  get(mem::Ptr8 ptr) const;
        u16 get(mem::Ptr16 ptr) const;
        u32 get(mem::Ptr32 ptr) const;
        u64 get(mem::Ptr64 ptr) const;
        f80 get(mem::Ptr80 ptr) const;
        Xmm get(mem::Ptr128 ptr) const;
        Xmm getUnaligned(mem::Ptr128 ptr) const;

        void set(mem::Ptr8 ptr, u8 value);
        void set(mem::Ptr16 ptr, u16 value);
        void set(mem::Ptr32 ptr, u32 value);
        void set(mem::Ptr64 ptr, u64 value);
        void set(mem::Ptr80 ptr, f80 value);
        void set(mem::Ptr128 ptr, Xmm value);
        void setUnaligned(mem::Ptr128 ptr, Xmm value);

        u8  xchg(mem::Ptr8 ptr, u8 value);
        u16 xchg(mem::Ptr16 ptr, u16 value);
        u32 xchg(mem::Ptr32 ptr, u32 value);
        u64 xchg(mem::Ptr64 ptr, u64 value);

        template<mem::Size size>
        inline U<size> get(const RM<size>& rm) const {
            return rm.isReg ? get(rm.reg) : get(resolve(rm.mem));
        }
        
        template<mem::Size size>
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
        
        template<mem::Size size>
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
        mem::Mmu* mmu_;
        Flags flags_;
        Registers regs_;
        X87Fpu x87fpu_;
        SimdControlStatus mxcsr_;
        std::array<u64, 8> segmentBase_ {{ 0, 0, 0, 0, 0, 0, 0, 0 }};

        SmallVector<Callback*, 2> callbacks_;

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

        template<mem::Size size>
        mem::SPtr<size> resolve(M<size> addr) const {
            return mem::SPtr<size>{getSegmentBase(addr.segment) + resolve(addr.encoding)};
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

        void execLockCmpxchg8Impl(mem::Ptr8 dst, u8 src);
        void execLockCmpxchg16Impl(mem::Ptr16 dst, u16 src);
        void execLockCmpxchg32Impl(mem::Ptr32 dst, u32 src);
        void execLockCmpxchg64Impl(mem::Ptr64 dst, u64 src);

        static const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> execFunctions_;

    public:
        void execAddRM8RM8(const Instruction&);
        void execAddRM8Imm(const Instruction&);
        void execAddRM16RM16(const Instruction&);
        void execAddRM16Imm(const Instruction&);
        void execAddRM32RM32(const Instruction&);
        void execAddRM32Imm(const Instruction&);
        void execAddRM64RM64(const Instruction&);
        void execAddRM64Imm(const Instruction&);

        void execLockAddM8RM8(const Instruction&);
        void execLockAddM8Imm(const Instruction&);
        void execLockAddM16RM16(const Instruction&);
        void execLockAddM16Imm(const Instruction&);
        void execLockAddM32RM32(const Instruction&);
        void execLockAddM32Imm(const Instruction&);
        void execLockAddM64RM64(const Instruction&);
        void execLockAddM64Imm(const Instruction&);

        void execAdcRM8RM8(const Instruction&);
        void execAdcRM8Imm(const Instruction&);
        void execAdcRM16RM16(const Instruction&);
        void execAdcRM16Imm(const Instruction&);
        void execAdcRM32RM32(const Instruction&);
        void execAdcRM32Imm(const Instruction&);
        void execAdcRM64RM64(const Instruction&);
        void execAdcRM64Imm(const Instruction&);

        void execSubRM8RM8(const Instruction&);
        void execSubRM8Imm(const Instruction&);
        void execSubRM16RM16(const Instruction&);
        void execSubRM16Imm(const Instruction&);
        void execSubRM32RM32(const Instruction&);
        void execSubRM32Imm(const Instruction&);
        void execSubRM64RM64(const Instruction&);
        void execSubRM64Imm(const Instruction&);

        void execLockSubM8RM8(const Instruction&);
        void execLockSubM8Imm(const Instruction&);
        void execLockSubM16RM16(const Instruction&);
        void execLockSubM16Imm(const Instruction&);
        void execLockSubM32RM32(const Instruction&);
        void execLockSubM32Imm(const Instruction&);
        void execLockSubM64RM64(const Instruction&);
        void execLockSubM64Imm(const Instruction&);

        void execSbbRM8RM8(const Instruction&);
        void execSbbRM8Imm(const Instruction&);
        void execSbbRM16RM16(const Instruction&);
        void execSbbRM16Imm(const Instruction&);
        void execSbbRM32RM32(const Instruction&);
        void execSbbRM32Imm(const Instruction&);
        void execSbbRM64RM64(const Instruction&);
        void execSbbRM64Imm(const Instruction&);

        void execNegRM8(const Instruction&);
        void execNegRM16(const Instruction&);
        void execNegRM32(const Instruction&);
        void execNegRM64(const Instruction&);

        void execMulRM8(const Instruction&);
        void execMulRM16(const Instruction&);
        void execMulRM32(const Instruction&);
        void execMulRM64(const Instruction&);

        void execImul1RM16(const Instruction&);
        void execImul2R16RM16(const Instruction&);
        void execImul3R16RM16Imm(const Instruction&);
        void execImul1RM32(const Instruction&);
        void execImul2R32RM32(const Instruction&);
        void execImul3R32RM32Imm(const Instruction&);
        void execImul1RM64(const Instruction&);
        void execImul2R64RM64(const Instruction&);
        void execImul3R64RM64Imm(const Instruction&);

        void execDivRM8(const Instruction&);
        void execDivRM16(const Instruction&);
        void execDivRM32(const Instruction&);
        void execDivRM64(const Instruction&);

        void execIdivRM32(const Instruction&);
        void execIdivRM64(const Instruction&);

        void execAndRM8RM8(const Instruction&);
        void execAndRM8Imm(const Instruction&);
        void execAndRM16RM16(const Instruction&);
        void execAndRM16Imm(const Instruction&);
        void execAndRM32RM32(const Instruction&);
        void execAndRM32Imm(const Instruction&);
        void execAndRM64RM64(const Instruction&);
        void execAndRM64Imm(const Instruction&);

        void execOrRM8RM8(const Instruction&);
        void execOrRM8Imm(const Instruction&);
        void execOrRM16RM16(const Instruction&);
        void execOrRM16Imm(const Instruction&);
        void execOrRM32RM32(const Instruction&);
        void execOrRM32Imm(const Instruction&);
        void execOrRM64RM64(const Instruction&);
        void execOrRM64Imm(const Instruction&);

        void execLockOrM8RM8(const Instruction&);
        void execLockOrM8Imm(const Instruction&);
        void execLockOrM16RM16(const Instruction&);
        void execLockOrM16Imm(const Instruction&);
        void execLockOrM32RM32(const Instruction&);
        void execLockOrM32Imm(const Instruction&);
        void execLockOrM64RM64(const Instruction&);
        void execLockOrM64Imm(const Instruction&);

        void execXorRM8RM8(const Instruction&);
        void execXorRM8Imm(const Instruction&);
        void execXorRM16RM16(const Instruction&);
        void execXorRM16Imm(const Instruction&);
        void execXorRM32RM32(const Instruction&);
        void execXorRM32Imm(const Instruction&);
        void execXorRM64RM64(const Instruction&);
        void execXorRM64Imm(const Instruction&);

        void execNotRM8(const Instruction&);
        void execNotRM16(const Instruction&);
        void execNotRM32(const Instruction&);
        void execNotRM64(const Instruction&);

        void execXchgRM8R8(const Instruction&);
        void execXchgRM16R16(const Instruction&);
        void execXchgRM32R32(const Instruction&);
        void execXchgRM64R64(const Instruction&);

        void execXaddRM8R8(const Instruction&);
        void execXaddRM16R16(const Instruction&);
        void execXaddRM32R32(const Instruction&);
        void execXaddRM64R64(const Instruction&);

        void execLockXaddM8R8(const Instruction&);
        void execLockXaddM16R16(const Instruction&);
        void execLockXaddM32R32(const Instruction&);
        void execLockXaddM64R64(const Instruction&);

        template<mem::Size size>
        void execMovRR(const Instruction&);

        void execMovMMXMMX(const Instruction&);

        template<mem::Size size>
        void execMovRM(const Instruction&);

        template<mem::Size size>
        void execMovMR(const Instruction&);

        template<mem::Size size>
        void execMovRImm(const Instruction&);

        template<mem::Size size>
        void execMovMImm(const Instruction&);

        void execMovq2dq(const Instruction&);
        void execMovdq2q(const Instruction&);

        void execMovaXMMM128(const Instruction&);
        void execMovaM128XMM(const Instruction&);
        void execMovuXMMM128(const Instruction&);
        void execMovuM128XMM(const Instruction&);

        void execMovsxR16RM8(const Instruction&);
        void execMovsxR32RM8(const Instruction&);
        void execMovsxR32RM16(const Instruction&);
        void execMovsxR64RM8(const Instruction&);
        void execMovsxR64RM16(const Instruction&);
        void execMovsxR64RM32(const Instruction&);

        void execMovzxR16RM8(const Instruction&);
        void execMovzxR32RM8(const Instruction&);
        void execMovzxR32RM16(const Instruction&);
        void execMovzxR64RM8(const Instruction&);
        void execMovzxR64RM16(const Instruction&);
        void execMovzxR64RM32(const Instruction&);

        void execLeaR32Encoding32(const Instruction&);
        void execLeaR64Encoding32(const Instruction&);
        void execLeaR32Encoding64(const Instruction&);
        void execLeaR64Encoding64(const Instruction&);

        void execPushImm(const Instruction&);
        void execPushRM32(const Instruction&);
        void execPushRM64(const Instruction&);

        void execPopR32(const Instruction&);
        void execPopR64(const Instruction&);
        void execPopM32(const Instruction&);
        void execPopM64(const Instruction&);

        void execPushfq(const Instruction&);
        void execPopfq(const Instruction&);

        void execCallDirect(const Instruction&);
        void execCallIndirectRM32(const Instruction&);
        void execCallIndirectRM64(const Instruction&);
        void execRet(const Instruction&);
        void execRetImm(const Instruction&);

        void execLeave(const Instruction&);
        void execHalt(const Instruction&);
        void execNop(const Instruction&);
        void execUd2(const Instruction&);
        void execSyscall(const Instruction&);
        void execUnknown(const Instruction&);

        void execCdq(const Instruction&);
        void execCqo(const Instruction&);

        void execIncRM8(const Instruction&);
        void execIncRM16(const Instruction&);
        void execIncRM32(const Instruction&);
        void execIncRM64(const Instruction&);

        void execLockIncM8(const Instruction&);
        void execLockIncM16(const Instruction&);
        void execLockIncM32(const Instruction&);
        void execLockIncM64(const Instruction&);

        void execDecRM8(const Instruction&);
        void execDecRM16(const Instruction&);
        void execDecRM32(const Instruction&);
        void execDecRM64(const Instruction&);

        void execLockDecM8(const Instruction&);
        void execLockDecM16(const Instruction&);
        void execLockDecM32(const Instruction&);
        void execLockDecM64(const Instruction&);

        void execShrRM8R8(const Instruction&);
        void execShrRM8Imm(const Instruction&);
        void execShrRM16R8(const Instruction&);
        void execShrRM16Imm(const Instruction&);
        void execShrRM32R8(const Instruction&);
        void execShrRM32Imm(const Instruction&);
        void execShrRM64R8(const Instruction&);
        void execShrRM64Imm(const Instruction&);

        void execShlRM8R8(const Instruction&);
        void execShlRM8Imm(const Instruction&);
        void execShlRM16R8(const Instruction&);
        void execShlRM16Imm(const Instruction&);
        void execShlRM32R8(const Instruction&);
        void execShlRM32Imm(const Instruction&);
        void execShlRM64R8(const Instruction&);
        void execShlRM64Imm(const Instruction&);

        void execShldRM32R32R8(const Instruction&);
        void execShldRM32R32Imm(const Instruction&);
        void execShldRM64R64R8(const Instruction&);
        void execShldRM64R64Imm(const Instruction&);

        void execShrdRM32R32R8(const Instruction&);
        void execShrdRM32R32Imm(const Instruction&);
        void execShrdRM64R64R8(const Instruction&);
        void execShrdRM64R64Imm(const Instruction&);

        void execSarRM8R8(const Instruction&);
        void execSarRM8Imm(const Instruction&);
        void execSarRM16R8(const Instruction&);
        void execSarRM16Imm(const Instruction&);
        void execSarRM32R8(const Instruction&);
        void execSarRM32Imm(const Instruction&);
        void execSarRM64R8(const Instruction&);
        void execSarRM64Imm(const Instruction&);

        void execSarxR32RM32R32(const Instruction&);
        void execSarxR64RM64R64(const Instruction&);
        void execShlxR32RM32R32(const Instruction&);
        void execShlxR64RM64R64(const Instruction&);
        void execShrxR32RM32R32(const Instruction&);
        void execShrxR64RM64R64(const Instruction&);

        void execRclRM8R8(const Instruction&);
        void execRclRM8Imm(const Instruction&);
        void execRclRM16R8(const Instruction&);
        void execRclRM16Imm(const Instruction&);
        void execRclRM32R8(const Instruction&);
        void execRclRM32Imm(const Instruction&);
        void execRclRM64R8(const Instruction&);
        void execRclRM64Imm(const Instruction&);

        void execRcrRM8R8(const Instruction&);
        void execRcrRM8Imm(const Instruction&);
        void execRcrRM16R8(const Instruction&);
        void execRcrRM16Imm(const Instruction&);
        void execRcrRM32R8(const Instruction&);
        void execRcrRM32Imm(const Instruction&);
        void execRcrRM64R8(const Instruction&);
        void execRcrRM64Imm(const Instruction&);

        void execRolRM8R8(const Instruction&);
        void execRolRM8Imm(const Instruction&);
        void execRolRM16R8(const Instruction&);
        void execRolRM16Imm(const Instruction&);
        void execRolRM32R8(const Instruction&);
        void execRolRM32Imm(const Instruction&);
        void execRolRM64R8(const Instruction&);
        void execRolRM64Imm(const Instruction&);

        void execRorRM8R8(const Instruction&);
        void execRorRM8Imm(const Instruction&);
        void execRorRM16R8(const Instruction&);
        void execRorRM16Imm(const Instruction&);
        void execRorRM32R8(const Instruction&);
        void execRorRM32Imm(const Instruction&);
        void execRorRM64R8(const Instruction&);
        void execRorRM64Imm(const Instruction&);

        void execTzcntR16RM16(const Instruction&);
        void execTzcntR32RM32(const Instruction&);
        void execTzcntR64RM64(const Instruction&);

        void execBtRM16R16(const Instruction&);
        void execBtRM16Imm(const Instruction&);
        void execBtRM32R32(const Instruction&);
        void execBtRM32Imm(const Instruction&);
        void execBtRM64R64(const Instruction&);
        void execBtRM64Imm(const Instruction&);

        void execBtrRM16R16(const Instruction&);
        void execBtrRM16Imm(const Instruction&);
        void execBtrRM32R32(const Instruction&);
        void execBtrRM32Imm(const Instruction&);
        void execBtrRM64R64(const Instruction&);
        void execBtrRM64Imm(const Instruction&);

        void execBtcRM16R16(const Instruction&);
        void execBtcRM16Imm(const Instruction&);
        void execBtcRM32R32(const Instruction&);
        void execBtcRM32Imm(const Instruction&);
        void execBtcRM64R64(const Instruction&);
        void execBtcRM64Imm(const Instruction&);

        void execBtsRM16R16(const Instruction&);
        void execBtsRM16Imm(const Instruction&);
        void execBtsRM32R32(const Instruction&);
        void execBtsRM32Imm(const Instruction&);
        void execBtsRM64R64(const Instruction&);
        void execBtsRM64Imm(const Instruction&);

        void execLockBtsM16R16(const Instruction&);
        void execLockBtsM16Imm(const Instruction&);
        void execLockBtsM32R32(const Instruction&);
        void execLockBtsM32Imm(const Instruction&);
        void execLockBtsM64R64(const Instruction&);
        void execLockBtsM64Imm(const Instruction&);

        void execTestRM8R8(const Instruction&);
        void execTestRM8Imm(const Instruction&);
        void execTestRM16R16(const Instruction&);
        void execTestRM16Imm(const Instruction&);
        void execTestRM32R32(const Instruction&);
        void execTestRM32Imm(const Instruction&);
        void execTestRM64R64(const Instruction&);
        void execTestRM64Imm(const Instruction&);

        void execCmpRM8RM8(const Instruction&);
        void execCmpRM8Imm(const Instruction&);
        void execCmpRM16RM16(const Instruction&);
        void execCmpRM16Imm(const Instruction&);
        void execCmpRM32RM32(const Instruction&);
        void execCmpRM32Imm(const Instruction&);
        void execCmpRM64RM64(const Instruction&);
        void execCmpRM64Imm(const Instruction&);

        void execCmpxchgRM8R8(const Instruction&);
        void execCmpxchgRM16R16(const Instruction&);
        void execCmpxchgRM32R32(const Instruction&);
        void execCmpxchgRM64R64(const Instruction&);
        void execCmpxchg16BM128(const Instruction&);

        void execLockCmpxchgM8R8(const Instruction&);
        void execLockCmpxchgM16R16(const Instruction&);
        void execLockCmpxchgM32R32(const Instruction&);
        void execLockCmpxchgM64R64(const Instruction&);
        void execLockCmpxchg16BM128(const Instruction&);

        void execSetRM8(const Instruction&);

        void execJmpRM32(const Instruction&);
        void execJmpRM64(const Instruction&);
        void execJmpu32(const Instruction&);
        void execJe(const Instruction&);
        void execJne(const Instruction&);
        void execJcc(const Instruction&);
        void execJrcxz(const Instruction&);

        void execBsrR16R16(const Instruction&);
        void execBsrR16M16(const Instruction&);
        void execBsrR32R32(const Instruction&);
        void execBsrR32M32(const Instruction&);
        void execBsrR64R64(const Instruction&);
        void execBsrR64M64(const Instruction&);
        
        void execBsfR16R16(const Instruction&);
        void execBsfR16M16(const Instruction&);
        void execBsfR32R32(const Instruction&);
        void execBsfR32M32(const Instruction&);
        void execBsfR64R64(const Instruction&);
        void execBsfR64M64(const Instruction&);

        void execCld(const Instruction&);
        void execStd(const Instruction&);

        void execMovsM8M8(const Instruction&);
        void execMovsM16M16(const Instruction&);
        void execMovsM64M64(const Instruction&);
        void execRepMovsM8M8(const Instruction&);
        void execRepMovsM16M16(const Instruction&);
        void execRepMovsM32M32(const Instruction&);
        void execRepMovsM64M64(const Instruction&);

        void execRepCmpsM8M8(const Instruction&);

        void execStosM8R8(const Instruction&);
        void execStosM16R16(const Instruction&);
        void execStosM32R32(const Instruction&);
        void execStosM64R64(const Instruction&);
        void execRepStosM8R8(const Instruction&);
        void execRepStosM16R16(const Instruction&);
        void execRepStosM32R32(const Instruction&);
        void execRepStosM64R64(const Instruction&);

        void execRepNZScasR8M8(const Instruction&);
        void execRepNZScasR16M16(const Instruction&);
        void execRepNZScasR32M32(const Instruction&);
        void execRepNZScasR64M64(const Instruction&);

        void execCmovR16RM16(const Instruction&);
        void execCmovR32RM32(const Instruction&);
        void execCmovR64RM64(const Instruction&);

        void execCbw(const Instruction&);
        void execCwde(const Instruction&);
        void execCdqe(const Instruction&);

        void execBswapR32(const Instruction&);
        void execBswapR64(const Instruction&);

        void execPopcntR16RM16(const Instruction&);
        void execPopcntR32RM32(const Instruction&);
        void execPopcntR64RM64(const Instruction&);

        void execMovapsXMMM128XMMM128(const Instruction&);

        void execMovdMMXRM32(const Instruction&);
        void execMovdRM32MMX(const Instruction&);
        void execMovdMMXRM64(const Instruction&);
        void execMovdRM64MMX(const Instruction&);

        void execMovdXMMRM32(const Instruction&);
        void execMovdRM32XMM(const Instruction&);
        void execMovdXMMRM64(const Instruction&);
        void execMovdRM64XMM(const Instruction&);

        void execMovqMMXRM64(const Instruction&);
        void execMovqRM64MMX(const Instruction&);
        void execMovqXMMRM64(const Instruction&);
        void execMovqRM64XMM(const Instruction&);

        void execFldz(const Instruction&);
        void execFld1(const Instruction&);
        void execFldlg2(const Instruction&);
        void execFldST(const Instruction&);
        void execFldM32(const Instruction&);
        void execFldM64(const Instruction&);
        void execFldM80(const Instruction&);
        void execFildM16(const Instruction&);
        void execFildM32(const Instruction&);
        void execFildM64(const Instruction&);
        void execFstpST(const Instruction&);
        void execFstpM32(const Instruction&);
        void execFstpM64(const Instruction&);
        void execFstpM80(const Instruction&);
        void execFistpM16(const Instruction&);
        void execFistpM32(const Instruction&);
        void execFistpM64(const Instruction&);
        void execFxchST(const Instruction&);

        void execFaddM32(const Instruction&);
        void execFaddM64(const Instruction&);
        void execFaddpST(const Instruction&);
        void execFsubSTM32(const Instruction&);
        void execFsubSTM64(const Instruction&);
        void execFsubSTST(const Instruction&);
        void execFsubpST(const Instruction&);
        void execFsubrpST(const Instruction&);
        void execFmul1M32(const Instruction&);
        void execFmul1M64(const Instruction&);
        void execFmulSTST(const Instruction&);
        void execFmulpSTST(const Instruction&);
        void execFdivSTST(const Instruction&);
        void execFdivM32(const Instruction&);
        void execFdivpSTST(const Instruction&);
        void execFdivrSTST(const Instruction&);
        void execFdivrM32(const Instruction&);
        void execFdivrpSTST(const Instruction&);

        void execFcomSTM32(const Instruction&);
        void execFcomSTM64(const Instruction&);
        void execFcomSTST(const Instruction&);
        void execFcompSTM32(const Instruction&);
        void execFcompSTM64(const Instruction&);
        void execFcompSTST(const Instruction&);
        void execFcomiSTST(const Instruction&);
        void execFcomipSTST(const Instruction&);
        void execFucomiSTST(const Instruction&);
        void execFucomipSTST(const Instruction&);
        void execFrndint(const Instruction&);

        void execFcmovST(const Instruction&);
        void execF2xm1(const Instruction&);
        void execFyl2x(const Instruction&);
        void execFscale(const Instruction&);
        void execFabs(const Instruction&);
        void execFchs(const Instruction&);

        void execFnstcwM16(const Instruction&);
        void execFldcwM16(const Instruction&);

        void execFnstswR16(const Instruction&);
        void execFnstswM16(const Instruction&);

        void execFnstenvM224(const Instruction&);
        void execFldenvM224(const Instruction&);

        void execFxam(const Instruction&);

        void execEmms(const Instruction&);

        void execMovssXMMM32(const Instruction&);
        void execMovssM32XMM(const Instruction&);
        void execMovssXMMXMM(const Instruction&);

        void execMovsdXMMM64(const Instruction&);
        void execMovsdM64XMM(const Instruction&);
        void execMovsdXMMXMM(const Instruction&);

        void execAddpsXMMXMMM128(const Instruction&);
        void execAddpdXMMXMMM128(const Instruction&);
        void execAddssXMMXMM(const Instruction&);
        void execAddssXMMM32(const Instruction&);
        void execAddsdXMMXMM(const Instruction&);
        void execAddsdXMMM64(const Instruction&);

        void execSubpsXMMXMMM128(const Instruction&);
        void execSubpdXMMXMMM128(const Instruction&);
        void execSubssXMMXMM(const Instruction&);
        void execSubssXMMM32(const Instruction&);
        void execSubsdXMMXMM(const Instruction&);
        void execSubsdXMMM64(const Instruction&);

        void execMulpsXMMXMMM128(const Instruction&);
        void execMulpdXMMXMMM128(const Instruction&);
        void execMulssXMMXMM(const Instruction&);
        void execMulssXMMM32(const Instruction&);
        void execMulsdXMMXMM(const Instruction&);
        void execMulsdXMMM64(const Instruction&);

        void execDivpsXMMXMMM128(const Instruction&);
        void execDivpdXMMXMMM128(const Instruction&);
        void execDivssXMMXMM(const Instruction&);
        void execDivssXMMM32(const Instruction&);
        void execDivsdXMMXMM(const Instruction&);
        void execDivsdXMMM64(const Instruction&);

        void execSqrtpsXMMXMMM128(const Instruction&);
        void execSqrtpdXMMXMMM128(const Instruction&);
        void execSqrtssXMMXMM(const Instruction&);
        void execSqrtssXMMM32(const Instruction&);
        void execSqrtsdXMMXMM(const Instruction&);
        void execSqrtsdXMMM64(const Instruction&);
        void execRsqrtssXMMXMM(const Instruction&);
        void execRsqrtssXMMM32(const Instruction&);
        void execRcppsXMMXMMM128(const Instruction&);

        void execComissXMMXMM(const Instruction&);
        void execComissXMMM32(const Instruction&);
        void execComisdXMMXMM(const Instruction&);
        void execComisdXMMM64(const Instruction&);
        void execUcomissXMMXMM(const Instruction&);
        void execUcomissXMMM32(const Instruction&);
        void execUcomisdXMMXMM(const Instruction&);
        void execUcomisdXMMM64(const Instruction&);

        void execMaxssXMMXMM(const Instruction&);
        void execMaxssXMMM32(const Instruction&);
        void execMaxsdXMMXMM(const Instruction&);
        void execMaxsdXMMM64(const Instruction&);

        void execMinssXMMXMM(const Instruction&);
        void execMinssXMMM32(const Instruction&);
        void execMinsdXMMXMM(const Instruction&);
        void execMinsdXMMM64(const Instruction&);

        void execMaxpsXMMXMMM128(const Instruction&);
        void execMaxpdXMMXMMM128(const Instruction&);

        void execMinpsXMMXMMM128(const Instruction&);
        void execMinpdXMMXMMM128(const Instruction&);

        void execCmpssXMMXMM(const Instruction&);
        void execCmpssXMMM32(const Instruction&);
        void execCmpsdXMMXMM(const Instruction&);
        void execCmpsdXMMM64(const Instruction&);
        void execCmppsXMMXMMM128(const Instruction&);
        void execCmppdXMMXMMM128(const Instruction&);

        void execCvtsi2ssXMMRM32(const Instruction&);
        void execCvtsi2ssXMMRM64(const Instruction&);
        void execCvtsi2sdXMMRM32(const Instruction&);
        void execCvtsi2sdXMMRM64(const Instruction&);

        void execCvtss2sdXMMXMM(const Instruction&);
        void execCvtss2sdXMMM32(const Instruction&);

        void execCvtss2siR32XMM(const Instruction&);
        void execCvtss2siR32M32(const Instruction&);
        void execCvtss2siR64XMM(const Instruction&);
        void execCvtss2siR64M32(const Instruction&);

        void execCvtsd2siR32XMM(const Instruction&);
        void execCvtsd2siR32M64(const Instruction&);
        void execCvtsd2siR64XMM(const Instruction&);
        void execCvtsd2siR64M64(const Instruction&);

        void execCvtsd2ssXMMXMM(const Instruction&);
        void execCvtsd2ssXMMM64(const Instruction&);

        void execCvttps2dqXMMXMMM128(const Instruction&);
        void execCvttpd2dqXMMXMMM128(const Instruction&);

        void execCvttss2siR32XMM(const Instruction&);
        void execCvttss2siR32M32(const Instruction&);
        void execCvttss2siR64XMM(const Instruction&);
        void execCvttss2siR64M32(const Instruction&);

        void execCvttsd2siR32XMM(const Instruction&);
        void execCvttsd2siR32M64(const Instruction&);
        void execCvttsd2siR64XMM(const Instruction&);
        void execCvttsd2siR64M64(const Instruction&);

        void execCvtdq2psXMMXMMM128(const Instruction&);
        void execCvtdq2pdXMMXMM(const Instruction&);
        void execCvtdq2pdXMMM64(const Instruction&);

        void execCvtps2dqXMMXMMM128(const Instruction&);

        void execCvtps2pdXMMXMM(const Instruction&);
        void execCvtps2pdXMMM64(const Instruction&);
        void execCvtpd2psXMMXMMM128(const Instruction&);

        void execStmxcsrM32(const Instruction&);
        void execLdmxcsrM32(const Instruction&);

        void execPandMMXMMXM64(const Instruction&);
        void execPandnMMXMMXM64(const Instruction&);
        void execPorMMXMMXM64(const Instruction&);
        void execPxorMMXMMXM64(const Instruction&);

        void execPandXMMXMMM128(const Instruction&);
        void execPandnXMMXMMM128(const Instruction&);
        void execPorXMMXMMM128(const Instruction&);
        void execPxorXMMXMMM128(const Instruction&);

        void execAndpdXMMXMMM128(const Instruction&);
        void execAndnpdXMMXMMM128(const Instruction&);
        void execOrpdXMMXMMM128(const Instruction&);
        void execXorpdXMMXMMM128(const Instruction&);

        void execShufpsXMMXMMM128Imm(const Instruction&);
        void execShufpdXMMXMMM128Imm(const Instruction&);

        void execMovlpsXMMM64(const Instruction&);
        void execMovlpsM64XMM(const Instruction&);
        void execMovhpsXMMM64(const Instruction&);
        void execMovhpsM64XMM(const Instruction&);
        void execMovhlpsXMMXMM(const Instruction&);
        void execMovlhpsXMMXMM(const Instruction&);

        void execPinsrwMMXR32Imm(const Instruction&);
        void execPinsrwMMXM16Imm(const Instruction&);

        void execPinsrwXMMR32Imm(const Instruction&);
        void execPinsrwXMMM16Imm(const Instruction&);
        void execPextrwR32XMMImm(const Instruction&);
        void execPextrwM16XMMImm(const Instruction&);

        void execPunpcklbwMMXMMXM32(const Instruction&);
        void execPunpcklwdMMXMMXM32(const Instruction&);
        void execPunpckldqMMXMMXM32(const Instruction&);
        void execPunpcklbwXMMXMMM128(const Instruction&);
        void execPunpcklwdXMMXMMM128(const Instruction&);
        void execPunpckldqXMMXMMM128(const Instruction&);
        void execPunpcklqdqXMMXMMM128(const Instruction&);

        void execPunpckhbwMMXMMXM64(const Instruction&);
        void execPunpckhwdMMXMMXM64(const Instruction&);
        void execPunpckhdqMMXMMXM64(const Instruction&);
        void execPunpckhbwXMMXMMM128(const Instruction&);
        void execPunpckhwdXMMXMMM128(const Instruction&);
        void execPunpckhdqXMMXMMM128(const Instruction&);
        void execPunpckhqdqXMMXMMM128(const Instruction&);

        void execPshufbMMXMMXM64(const Instruction&);
        void execPshufbXMMXMMM128(const Instruction&);
        void execPshufwMMXMMXM64Imm(const Instruction&);
        void execPshuflwXMMXMMM128Imm(const Instruction&);
        void execPshufhwXMMXMMM128Imm(const Instruction&);
        void execPshufdXMMXMMM128Imm(const Instruction&);

        void execPcmpeqbMMXMMXM64(const Instruction&);
        void execPcmpeqwMMXMMXM64(const Instruction&);
        void execPcmpeqdMMXMMXM64(const Instruction&);

        void execPcmpeqbXMMXMMM128(const Instruction&);
        void execPcmpeqwXMMXMMM128(const Instruction&);
        void execPcmpeqdXMMXMMM128(const Instruction&);
        void execPcmpeqqXMMXMMM128(const Instruction&);

        void execPcmpgtbMMXMMXM64(const Instruction&);
        void execPcmpgtwMMXMMXM64(const Instruction&);
        void execPcmpgtdMMXMMXM64(const Instruction&);

        void execPcmpgtbXMMXMMM128(const Instruction&);
        void execPcmpgtwXMMXMMM128(const Instruction&);
        void execPcmpgtdXMMXMMM128(const Instruction&);
        void execPcmpgtqXMMXMMM128(const Instruction&);

        void execPmovmskbR32MMX(const Instruction&);
        void execPmovmskbR64MMX(const Instruction&);
        void execPmovmskbR32XMM(const Instruction&);
        void execPmovmskbR64XMM(const Instruction&);

        void execPaddbMMXMMXM64(const Instruction&);
        void execPaddwMMXMMXM64(const Instruction&);
        void execPadddMMXMMXM64(const Instruction&);
        void execPaddqMMXMMXM64(const Instruction&);
        void execPaddsbMMXMMXM64(const Instruction&);
        void execPaddswMMXMMXM64(const Instruction&);
        void execPaddusbMMXMMXM64(const Instruction&);
        void execPadduswMMXMMXM64(const Instruction&);

        void execPaddbXMMXMMM128(const Instruction&);
        void execPaddwXMMXMMM128(const Instruction&);
        void execPadddXMMXMMM128(const Instruction&);
        void execPaddqXMMXMMM128(const Instruction&);
        void execPaddsbXMMXMMM128(const Instruction&);
        void execPaddswXMMXMMM128(const Instruction&);
        void execPaddusbXMMXMMM128(const Instruction&);
        void execPadduswXMMXMMM128(const Instruction&);

        void execPsubbMMXMMXM64(const Instruction&);
        void execPsubwMMXMMXM64(const Instruction&);
        void execPsubdMMXMMXM64(const Instruction&);
        void execPsubqMMXMMXM64(const Instruction&);
        void execPsubsbMMXMMXM64(const Instruction&);
        void execPsubswMMXMMXM64(const Instruction&);
        void execPsubusbMMXMMXM64(const Instruction&);
        void execPsubuswMMXMMXM64(const Instruction&);

        void execPsubbXMMXMMM128(const Instruction&);
        void execPsubwXMMXMMM128(const Instruction&);
        void execPsubdXMMXMMM128(const Instruction&);
        void execPsubqXMMXMMM128(const Instruction&);
        void execPsubsbXMMXMMM128(const Instruction&);
        void execPsubswXMMXMMM128(const Instruction&);
        void execPsubusbXMMXMMM128(const Instruction&);
        void execPsubuswXMMXMMM128(const Instruction&);

        void execPmulhuwMMXMMXM64(const Instruction&);
        void execPmulhwMMXMMXM64(const Instruction&);
        void execPmullwMMXMMXM64(const Instruction&);
        void execPmuludqMMXMMXM64(const Instruction&);

        void execPmulhuwXMMXMMM128(const Instruction&);
        void execPmulhwXMMXMMM128(const Instruction&);
        void execPmullwXMMXMMM128(const Instruction&);
        void execPmuludqXMMXMMM128(const Instruction&);

        void execPmaddwdMMXMMXM64(const Instruction&);
        void execPmaddwdXMMXMMM128(const Instruction&);

        void execPsadbwMMXMMXM64(const Instruction&);
        void execPsadbwXMMXMMM128(const Instruction&);

        void execPavgbMMXMMXM64(const Instruction&);
        void execPavgwMMXMMXM64(const Instruction&);
        void execPavgbXMMXMMM128(const Instruction&);
        void execPavgwXMMXMMM128(const Instruction&);

        void execPmaxswMMXMMXM64(const Instruction&);
        void execPmaxswXMMXMMM128(const Instruction&);
        void execPmaxubMMXMMXM64(const Instruction&);
        void execPmaxubXMMXMMM128(const Instruction&);

        void execPminswMMXMMXM64(const Instruction&);
        void execPminswXMMXMMM128(const Instruction&);
        void execPminubMMXMMXM64(const Instruction&);
        void execPminubXMMXMMM128(const Instruction&);

        void execPtestXMMXMMM128(const Instruction&);

        void execPsrawMMXImm(const Instruction&);
        void execPsrawMMXMMXM64(const Instruction&);
        void execPsradMMXImm(const Instruction&);
        void execPsradMMXMMXM64(const Instruction&);

        void execPsrawXMMImm(const Instruction&);
        void execPsrawXMMXMMM128(const Instruction&);
        void execPsradXMMImm(const Instruction&);
        void execPsradXMMXMMM128(const Instruction&);

        void execPsllwMMXImm(const Instruction&);
        void execPsllwMMXMMXM64(const Instruction&);
        void execPslldMMXImm(const Instruction&);
        void execPslldMMXMMXM64(const Instruction&);
        void execPsllqMMXImm(const Instruction&);
        void execPsllqMMXMMXM64(const Instruction&);
        void execPsrlwMMXImm(const Instruction&);
        void execPsrlwMMXMMXM64(const Instruction&);
        void execPsrldMMXImm(const Instruction&);
        void execPsrldMMXMMXM64(const Instruction&);
        void execPsrlqMMXImm(const Instruction&);
        void execPsrlqMMXMMXM64(const Instruction&);

        void execPsllwXMMImm(const Instruction&);
        void execPsllwXMMXMMM128(const Instruction&);
        void execPslldXMMImm(const Instruction&);
        void execPslldXMMXMMM128(const Instruction&);
        void execPsllqXMMImm(const Instruction&);
        void execPsllqXMMXMMM128(const Instruction&);
        void execPsrlwXMMImm(const Instruction&);
        void execPsrlwXMMXMMM128(const Instruction&);
        void execPsrldXMMImm(const Instruction&);
        void execPsrldXMMXMMM128(const Instruction&);
        void execPsrlqXMMImm(const Instruction&);
        void execPsrlqXMMXMMM128(const Instruction&);

        void execPslldqXMMImm(const Instruction&);
        void execPsrldqXMMImm(const Instruction&);

        void execPackuswbMMXMMXM64(const Instruction&);
        void execPacksswbMMXMMXM64(const Instruction&);
        void execPackssdwMMXMMXM64(const Instruction&);

        void execPackuswbXMMXMMM128(const Instruction&);
        void execPackusdwXMMXMMM128(const Instruction&);
        void execPacksswbXMMXMMM128(const Instruction&);
        void execPackssdwXMMXMMM128(const Instruction&);

        void execUnpckhpsXMMXMMM128(const Instruction&);
        void execUnpckhpdXMMXMMM128(const Instruction&);
        void execUnpcklpsXMMXMMM128(const Instruction&);
        void execUnpcklpdXMMXMMM128(const Instruction&);

        void execMovmskpsR32XMM(const Instruction&);
        void execMovmskpsR64XMM(const Instruction&);
        void execMovmskpdR32XMM(const Instruction&);
        void execMovmskpdR64XMM(const Instruction&);

        void execLddquXMMM128(const Instruction&);
        void execMovshdupXMMXMMM128(const Instruction&);
        void execMovddupXMMXMM(const Instruction&);
        void execMovddupXMMM64(const Instruction&);
        void execAddsubpsXMXMMM128(const Instruction&);
        void execAddsubpdXMXMMM128(const Instruction&);
        void execHaddpsXMXMMM128(const Instruction&);
        void execHaddpdXMXMMM128(const Instruction&);

        void execPalignrMMXMMXM64Imm(const Instruction&);
        void execPalignrXMMXMMM128Imm(const Instruction&);

        void execPhaddwMMXMMXM64(const Instruction&);
        void execPhaddwXMXXMXM128(const Instruction&);
        void execPhadddMMXMMXM64(const Instruction&);
        void execPhadddXMXXMXM128(const Instruction&);
        void execPmaddubswMMXMMXM64(const Instruction&);
        void execPmaddubswXMMXMMM128(const Instruction&);

        void execPmulhrswMMXMMXM64(const Instruction&);
        void execPmulhrswXMMXMMM128(const Instruction&);

        void execPabsbMMXMMXM64(const Instruction&);
        void execPabswMMXMMXM64(const Instruction&);
        void execPabsdMMXMMXM64(const Instruction&);
        void execPabsbXMMXMMM128(const Instruction&);
        void execPabswXMMXMMM128(const Instruction&);
        void execPabsdXMMXMMM128(const Instruction&);

        void execPsignbMMXMMXM64(const Instruction&);
        void execPsignwMMXMMXM64(const Instruction&);
        void execPsigndMMXMMXM64(const Instruction&);
        void execPsignbXMMXMMM128(const Instruction&);
        void execPsignwXMMXMMM128(const Instruction&);
        void execPsigndXMMXMMM128(const Instruction&);

        void execPmaxuwXMMXMMM128(const Instruction&);
        void execPmaxudXMMXMMM128(const Instruction&);
        void execPminuwXMMXMMM128(const Instruction&);
        void execPminudXMMXMMM128(const Instruction&);
        void execPmaxsbXMMXMMM128(const Instruction&);
        void execPmaxsdXMMXMMM128(const Instruction&);
        void execPminsbXMMXMMM128(const Instruction&);
        void execPminsdXMMXMMM128(const Instruction&);

        void execPmovzxbwXMMXMM(const Instruction&);
        void execPmovzxbdXMMXMM(const Instruction&);
        void execPmovzxbqXMMXMM(const Instruction&);
        void execPmovzxwdXMMXMM(const Instruction&);
        void execPmovzxwqXMMXMM(const Instruction&);
        void execPmovzxdqXMMXMM(const Instruction&);
        void execPmovzxbwXMMM64(const Instruction&);
        void execPmovzxbdXMMM32(const Instruction&);
        void execPmovzxbqXMMM16(const Instruction&);
        void execPmovzxwdXMMM64(const Instruction&);
        void execPmovzxwqXMMM32(const Instruction&);
        void execPmovzxdqXMMM64(const Instruction&);

        void execPmovsxbwXMMXMM(const Instruction&);
        void execPmovsxbdXMMXMM(const Instruction&);
        void execPmovsxbqXMMXMM(const Instruction&);
        void execPmovsxwdXMMXMM(const Instruction&);
        void execPmovsxwqXMMXMM(const Instruction&);
        void execPmovsxdqXMMXMM(const Instruction&);
        void execPmovsxbwXMMM64(const Instruction&);
        void execPmovsxbdXMMM32(const Instruction&);
        void execPmovsxbqXMMM16(const Instruction&);
        void execPmovsxwdXMMM64(const Instruction&);
        void execPmovsxwqXMMM32(const Instruction&);
        void execPmovsxdqXMMM64(const Instruction&);

        void execRoundssXMMXMMImm(const Instruction&);
        void execRoundssXMMM32Imm(const Instruction&);
        void execRoundsdXMMXMMImm(const Instruction&);
        void execRoundsdXMMM64Imm(const Instruction&);
        void execRoundpsXMMXMMImm(const Instruction&);
        void execRoundpdXMMXMMImm(const Instruction&);

        void execPmulldXMMXMMM128(const Instruction&);
        void execPextrbR32XMMImm(const Instruction&);
        void execPextrbM8XMMImm(const Instruction&);
        void execPextrdRM32XMMImm(const Instruction&);
        void execPextrqRM64XMMImm(const Instruction&);
        void execPinsrbXMMR32Imm(const Instruction&);
        void execPinsrdXMMRM32Imm(const Instruction&);
        void execPinsrqXMMRM64Imm(const Instruction&);
        void execExtractpsM32XMMImm(const Instruction&);
        void execInsertpsXMMXMMImm(const Instruction&);
        void execBlendpsXMMXMMM128Imm(const Instruction&);
        void execBlendpdXMMXMMM128Imm(const Instruction&);
        void execBlendvpsXMMXMMM128(const Instruction&);
        void execBlendvpdXMMXMMM128(const Instruction&);
        void execPblendvbXMMXMMM128(const Instruction&);
        void execPblendwXMMM128Imm(const Instruction&);
        
        void execPcmpistriXMMXMMM128Imm(const Instruction&);
        void execPcmpestriXMMXMMM128Imm(const Instruction&);
        void execCrc32R32RM8(const Instruction&);
        void execCrc32R32RM16(const Instruction&);
        void execCrc32R32RM32(const Instruction&);
        void execCrc32R64RM64(const Instruction&);

        void execRdtsc(const Instruction&);

        void execCpuid(const Instruction&);
        void execXgetbv(const Instruction&);

        void execFxsaveM4096(const Instruction&);
        void execFxrstorM4096(const Instruction&);

        void execFwait(const Instruction&);

        void execRdpkru(const Instruction&);
        void execWrpkru(const Instruction&);

        void execRdsspd(const Instruction&);

        void execPause(const Instruction&);

        void execUnimplemented(const Instruction&);

    };

}

#endif
