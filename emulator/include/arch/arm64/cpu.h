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

        u64 get(R64 reg) const { return regs_.get(reg); }
        void set(R64 reg, u64 value) { regs_.set(reg, value); }


        static BasicBlock createBasicBlock(const Instruction* instructions, size_t count);

        void exec(const BasicBlock&);

        void execUnknown(const Instruction& ins);

    private:
        mem::Mmu& mmu_;
        Registers regs_;
        Flags flags_;

        SmallVector<Callback*, 2> callbacks_;

        static const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> execFunctions_;
    };

}

#endif