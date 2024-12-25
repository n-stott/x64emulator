#ifndef VM_H
#define VM_H

#include "emulator/symbolprovider.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "utils.h"
#include <deque>
#include <string>
#include <unordered_map>

namespace kernel {
    class Kernel;
    class Thread;
}

namespace emulator {

    class VM {
    public:
        explicit VM(x64::Mmu& mmu, kernel::Kernel& kernel);
        ~VM();

        void crash();
        bool hasCrashed() const { return hasCrashed_; }

        void contextSwitch(kernel::Thread* newThread);

        void execute(kernel::Thread* thread);

        void push64(u64 value);

        void tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) const;

        class MmuCallback : public x64::Mmu::Callback {
        public:
            MmuCallback(x64::Mmu* mmu, VM* vm);
            ~MmuCallback();
            void on_mprotect(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) override;
            void on_munmap(u64 base, u64 length) override;
        private:
            x64::Mmu* mmu_ { nullptr };
            VM* vm_ { nullptr };
        };

    private:
        friend class x64::Cpu;
        friend class kernel::Sys;

        void log(size_t ticks, const x64::X64Instruction& instruction) const;

        struct BBlock;

        BBlock* fetchBasicBlock();

        void notifyCall(u64 address);
        void notifyRet();

        void syncThread();
        void enterSyscall();

        struct ExecutableSection {
            u64 begin;
            u64 end;
            std::vector<x64::X64Instruction> instructions;
            std::string filename;

            void trim();
        };

        struct InstructionPosition {
            const ExecutableSection* section;
            size_t index;
        };

        InstructionPosition findSectionWithAddress(u64 address, const ExecutableSection* sectionHint = nullptr) const;
        std::string callName(const x64::X64Instruction& instruction) const;
        std::string calledFunctionName(u64 address) const;

        x64::Mmu& mmu_;
        kernel::Kernel& kernel_;
        x64::Cpu cpu_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;

        kernel::Thread* currentThread_ { nullptr };

        struct BBlock {
            x64::Cpu::BasicBlock cpuBasicBlock;
            std::array<BBlock*, 2> cachedDestinations {{ nullptr, nullptr }};

            std::optional<u64> start() const {
                if(cpuBasicBlock.instructions.empty()) return {};
                return cpuBasicBlock.instructions[0].first.address();
            }
            std::optional<u64> end() const {
                if(cpuBasicBlock.instructions.empty()) return {};
                return cpuBasicBlock.instructions.back().first.nextAddress();
            }
        };

        std::unordered_map<u64, std::unique_ptr<BBlock>> basicBlocks_;

#ifdef VM_BASICBLOCK_TELEMETRY
        u64 blockCacheHits_ { 0 };
        u64 blockCacheMisses_ { 0 };
        std::unordered_map<u64, u64> basicBlockCount_;
#endif

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

        friend class MunmapCallback;
    };

}

#endif