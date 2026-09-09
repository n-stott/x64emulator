#ifndef ARM64CPU_H
#define ARM64CPU_H

#include "arch/arm64/instructions/basicblock.h"
#include "arch/arm64/registers.h"
#include "arch/arm64/flags.h"
#include "mem/mmu.h"
#include "smallvector.h"
#include "utils.h"

namespace arm64 {

    class Cpu;
    using CpuExecPtr = void(*)(Cpu&, const Instruction&);

    class Cpu {
    public:
        explicit Cpu(mem::Mmu&);

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

        struct State {
            Flags flags;
            Registers regs;
        };

        void save(State*) const;
        void load(const State&);

        u32 get(R32 reg) const { return regs_.get(reg); }
        void set(R32 reg, u32 value) { regs_.set(reg, value); }
        u64 get(R64 reg) const { return regs_.get(reg); }
        void set(R64 reg, u64 value) { regs_.set(reg, value); }
        u64 get(D64 reg) const { return regs_.get(reg); }
        void set(D64 reg, u64 value) { regs_.set(reg, value); }
        u128 get(Q128 reg) const { return regs_.get(reg); }
        void set(Q128 reg, u128 value) { regs_.set(reg, value); }
        u128 get(V128 reg) const { return regs_.get(reg); }
        void set(V128 reg, u128 value) { regs_.set(reg, value); }

        u32 get(const ShiftedR32& reg) const {
            u32 val = regs_.get(reg.reg);
            if(reg.lsl) val <<= reg.lsl;
            if(reg.lsr) val >>= reg.lsr;
            if(reg.asr) {
                warn("asr not handled");
                val >>= reg.asr;
            }
            return val;
        }

        u64 get(const ShiftedR64& reg) const {
            u64 val = regs_.get(reg.reg);
            if(reg.lsl) val <<= reg.lsl;
            if(reg.lsr) val >>= reg.lsr;
            if(reg.asr) {
                warn("asr not handled");
                val >>= reg.asr;
            }
            return val;
        }

        static BasicBlock createBasicBlock(const Instruction* instructions, size_t count);

        void exec(const BasicBlock&);

        void execNop(const Instruction& ins);
        void execMovR32Imm(const Instruction& ins);
        void execMovR32R32(const Instruction& ins);
        void execMovR64Imm(const Instruction& ins);
        void execMovR64R64(const Instruction& ins);
        void execMovR64SImm(const Instruction& ins);
        void execMoviD64Imm(const Instruction& ins);
        void execMovkR32Imm(const Instruction& ins);
        void execMovkR32SImm(const Instruction& ins);
        void execMovkR64Imm(const Instruction& ins);
        void execMovkR64SImm(const Instruction& ins);
        void execMovnR32Imm(const Instruction& ins);
        void execMovnR64Imm(const Instruction& ins);
        void execMovzR32Imm(const Instruction& ins);
        void execMovzR64Imm(const Instruction& ins);
        void execMovzR32SImm(const Instruction& ins);
        void execMovzR64SImm(const Instruction& ins);
        void execMrsR64Sysreg(const Instruction& ins);
        void execMsrSysregR64(const Instruction& ins);
        void execLdrR32M32(const Instruction& ins);
        void execLdrR64M64(const Instruction& ins);
        void execLdrR64M64Imm(const Instruction& ins);
        void execLdrQ128M128(const Instruction& ins);
        void execLdrbR32M8(const Instruction& ins);
        void execLdrhR32M16(const Instruction& ins);
        void execLdrhR32M16Imm(const Instruction& ins);
        void execLdarR32M32(const Instruction& ins);
        void execLdarR64M64(const Instruction& ins);
        void execLdxrR32M32(const Instruction& ins);
        void execLdxrR64M64(const Instruction& ins);
        void execLdaxrR32M32(const Instruction& ins);
        void execLdaxrR64M64(const Instruction& ins);
        void execLdpR32R32M64(const Instruction& ins);
        void execLdpR64R64M128(const Instruction& ins);
        void execLdpR64R64M128Imm(const Instruction& ins);
        void execLdpQ128Q128M256(const Instruction& ins);
        void execStrbR32M8(const Instruction& ins);
        void execStrhR32M16(const Instruction& ins);
        void execStrR32M32(const Instruction& ins);
        void execStrR32M32Imm(const Instruction& ins);
        void execStrR64M64(const Instruction& ins);
        void execStrR64M64Imm(const Instruction& ins);
        void execStrD64M64(const Instruction& ins);
        void execStrQ128M128(const Instruction& ins);
        void execStlrR32M32(const Instruction& ins);
        void execStlrR64M64(const Instruction& ins);
        void execStxrR32R32M32(const Instruction& ins);
        void execStxrR32R64M64(const Instruction& ins);
        void execStlxrR32R32M32(const Instruction& ins);
        void execStlxrR32R64M64(const Instruction& ins);
        void execStpR32R32M64(const Instruction& ins);
        void execStpR64R64M128(const Instruction& ins);
        void execStpD64D64M128(const Instruction& ins);
        void execStpQ128Q128M256(const Instruction& ins);
        void execAddR32R32R32(const Instruction& ins);
        void execAddR32R32Imm(const Instruction& ins);
        void execAddR32R32SR32(const Instruction& ins);
        void execAddR64R64R64(const Instruction& ins);
        void execAddR64R64Imm(const Instruction& ins);
        void execAddR64R64R32sxtwImm(const Instruction& ins);
        void execAddR64R64R32uxtwImm(const Instruction& ins);
        void execAddR64R64SImm(const Instruction& ins);
        void execAddR64R64SR64(const Instruction& ins);
        void execAddsR32R32R32(const Instruction& ins);
        void execAddsR32R32Imm(const Instruction& ins);
        void execAddsR64R64R64(const Instruction& ins);
        void execAddsR64R64Imm(const Instruction& ins);
        void execSubR32R32R32(const Instruction& ins);
        void execSubR32R32SImm(const Instruction& ins);
        void execSubR64R64SR64(const Instruction& ins);
        void execSubR64R64SImm(const Instruction& ins);
        void execSubR64R64R32uxtw(const Instruction& ins);
        void execSubsR32R32R32(const Instruction& ins);
        void execSubsR32R32SImm(const Instruction& ins);
        void execSubsR64R64R64(const Instruction& ins);
        void execSubsR64R64SImm(const Instruction& ins);
        void execMulR32R32R32(const Instruction& ins);
        void execMulR64R64R64(const Instruction& ins);
        void execUdivR32R32R32(const Instruction& ins);
        void execUdivR64R64R64(const Instruction& ins);
        void execMaddR32R32R32R32(const Instruction& ins);
        void execMaddR64R64R64R64(const Instruction& ins);
        void execMsubR32R32R32R32(const Instruction& ins);
        void execMsubR64R64R64R64(const Instruction& ins);
        void execUmaddlR64R32R32R64(const Instruction& ins);
        void execNegR32R32(const Instruction& ins);
        void execNegR64R64(const Instruction& ins);
        void execAndR32R32R32(const Instruction& ins);
        void execAndR32R32Imm(const Instruction& ins);
        void execAndR64R64R64(const Instruction& ins);
        void execAndR64R64Imm(const Instruction& ins);
        void execAndsR32R32R32(const Instruction& ins);
        void execAndsR32R32Imm(const Instruction& ins);
        void execAndsR64R64R64(const Instruction& ins);
        void execAndsR64R64Imm(const Instruction& ins);
        void execBicR32R32R32(const Instruction& ins);
        void execBicR64R64R64(const Instruction& ins);
        void execBicsR32R32R32(const Instruction& ins);
        void execBicsR64R64R64(const Instruction& ins);
        void execOrrR32R32R32(const Instruction& ins);
        void execOrrR32R32Imm(const Instruction& ins);
        void execOrrR64R64R64(const Instruction& ins);
        void execOrrR64R64Imm(const Instruction& ins);
        void execOrrR32R32SR32(const Instruction& ins);
        void execOrrR32R32SImm(const Instruction& ins);
        void execOrrR64R64SR64(const Instruction& ins);
        void execOrrR64R64SImm(const Instruction& ins);
        void execEorR32R32R32(const Instruction& ins);
        void execEorR32R32Imm(const Instruction& ins);
        void execEorR64R64R64(const Instruction& ins);
        void execEorR64R64Imm(const Instruction& ins);
        void execEorR32R32SR32(const Instruction& ins);
        void execEorR32R32SImm(const Instruction& ins);
        void execEorR64R64SR64(const Instruction& ins);
        void execEorR64R64SImm(const Instruction& ins);
        void execLslR32R32R32(const Instruction& ins);
        void execLslR32R32Imm(const Instruction& ins);
        void execLslR64R64R64(const Instruction& ins);
        void execLslR64R64Imm(const Instruction& ins);
        void execLsrR32R32R32(const Instruction& ins);
        void execLsrR32R32Imm(const Instruction& ins);
        void execLsrR64R64R64(const Instruction& ins);
        void execLsrR64R64Imm(const Instruction& ins);
        void execAsrR32R32R32(const Instruction& ins);
        void execAsrR32R32Imm(const Instruction& ins);
        void execAsrR64R64R64(const Instruction& ins);
        void execAsrR64R64Imm(const Instruction& ins);
        void execClzR32R32(const Instruction& ins);
        void execClzR64R64(const Instruction& ins);
        void execRevR32R32(const Instruction& ins);
        void execRevR64R64(const Instruction& ins);
        void execRbitR32R32(const Instruction& ins);
        void execRbitR64R64(const Instruction& ins);
        void execSbfizR64R64ImmImm(const Instruction& ins);
        void execUbfizR64R64ImmImm(const Instruction& ins);
        void execUbfxR32R32ImmImm(const Instruction& ins);
        void execUbfxR64R64ImmImm(const Instruction& ins);
        void execSxtwR64R32(const Instruction& ins);
        void execAdrpR64Imm(const Instruction& ins);
        void execTstR32R32(const Instruction& ins);
        void execTstR32Imm(const Instruction& ins);
        void execTstR64R64(const Instruction& ins);
        void execTstR64Imm(const Instruction& ins);
        void execCmpR32R32(const Instruction& ins);
        void execCmpR64R64(const Instruction& ins);
        void execCmpR32Imm(const Instruction& ins);
        void execCmpR64Imm(const Instruction& ins);
        void execCmpR32SImm(const Instruction& ins);
        void execCmpR64SImm(const Instruction& ins);
        void execCcmpR32R32ImmCond(const Instruction& ins);
        void execCcmpR32ImmImmCond(const Instruction& ins);
        void execCcmpR64R64ImmCond(const Instruction& ins);
        void execCcmpR64ImmImmCond(const Instruction& ins);
        void execCmnR32R32(const Instruction& ins);
        void execCmnR64R64(const Instruction& ins);
        void execCmnR32Imm(const Instruction& ins);
        void execCmnR64Imm(const Instruction& ins);
        void execCmnR32SImm(const Instruction& ins);
        void execCmnR64SImm(const Instruction& ins);
        void execCsetR32Cond(const Instruction& ins);
        void execCsetR64Cond(const Instruction& ins);
        void execCsetmR32Cond(const Instruction& ins);
        void execCsetmR64Cond(const Instruction& ins);
        void execCselR32R32R32Cond(const Instruction& ins);
        void execCselR64R64R64Cond(const Instruction& ins);
        void execCsincR32R32R32Cond(const Instruction& ins);
        void execCsincR64R64R64Cond(const Instruction& ins);
        void execCsinvR32R32R32Cond(const Instruction& ins);
        void execCsinvR64R64R64Cond(const Instruction& ins);
        void execCasaR32R32M32(const Instruction& ins);
        void execCasaR64R64M64(const Instruction& ins);
        void execSwplR32R32M32(const Instruction& ins);
        void execSwplR64R64M64(const Instruction& ins);
        void execBImm(const Instruction& ins);
        void execBrR64(const Instruction& ins);
        void execBlImm(const Instruction& ins);
        void execBlrR64(const Instruction& ins);
        void execBCondImm(const Instruction& ins);
        void execCbzR32Imm(const Instruction& ins);
        void execCbzR64Imm(const Instruction& ins);
        void execCbnzR32Imm(const Instruction& ins);
        void execCbnzR64Imm(const Instruction& ins);
        void execTbzR32ImmImm(const Instruction& ins);
        void execTbzR64ImmImm(const Instruction& ins);
        void execTbnzR32ImmImm(const Instruction& ins);
        void execTbnzR64ImmImm(const Instruction& ins);
        void execRet(const Instruction& ins);
        void execSvcImm(const Instruction& ins);
        void execDcSysopR64(const Instruction& ins);
    // simd
        void execMoviV4sImm(const Instruction& ins);
        void execMoviV16bImm(const Instruction& ins);
        void execMvniV4SImm(const Instruction& ins);
        void execLd1V16bM128(const Instruction& ins);
        void execDupV16bR32(const Instruction& ins);
        void execShrnB8bV8hImm(const Instruction& ins);
        void execExtV16bV16bV16bImm(const Instruction& ins);
        void execBitV16bV16bV16b(const Instruction& ins);
        void execUmaxpV16bV16bV16b(const Instruction& ins);
        void execCmCondV16bV16bZero(const Instruction& ins);
        void execCmCondV16bV16bV16b(const Instruction& ins);
    // float
        void execFmovR64D64(const Instruction& ins);
        void execUnknown(const Instruction& ins);

    private:
        mem::Mmu& mmu_;
        Registers regs_;
        Flags flags_;

        template<mem::Size size>
        u64 resolveAddress(const M<size>& address);

        mem::Ptr8 resolve(const M8&);
        mem::Ptr16 resolve(const M16&);
        mem::Ptr32 resolve(const M32&);
        mem::Ptr64 resolve(const M64&);
        mem::Ptr128 resolve(const M128&);

        SmallVector<Callback*, 2> callbacks_;

        static const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> execFunctions_;
    };

}

#endif