#ifndef ARM64VM_H
#define ARM64VM_H

#include "arch/arm64/codesegment.h"
#include "arch/arm64/cpu.h"
#include "linux-arm64-emulator/vmthread.h"
#include "mem/mmu.h"
#include "intervalvector.h"
#include "utils.h"
#include <deque>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace arm64 {
    class BasicBlock;
    class CodeSegment;
    class Compiler;
    class Disassembler;
    class JitBasicBlock;
    class Jit;
    class JitStats;

    class VM {
    public:
        explicit VM(mem::Mmu& mmu);
        ~VM();

        void execute(VMThread* thread);

        class CpuCallback : public Cpu::Callback {
        public:
            CpuCallback(Cpu* cpu, VM* vm);
            ~CpuCallback();
            void onSyscall() override;
            void onCall(u64 address) override;
            void onRet() override;
            void onStackChange(u64 stackptr) override;
        private:
            Cpu* cpu_ { nullptr };
            VM* vm_ { nullptr };
        };

    private:
        void notifyCall(u64 address);
        void notifyRet();
        void notifyStackChange(u64 stackptr);

        void contextSwitch(VMThread* newThread);
        void syncThread();
        void enterSyscall();

        Cpu cpu_;
        mem::Mmu& mmu_;

        VMThread* currentThread_ { nullptr };
    };

}

#endif