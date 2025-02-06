#ifndef VM_H
#define VM_H

#include "emulator/symbolprovider.h"
#include "x64/compiler/compiler.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "utils.h"
#include <deque>
#include <unordered_set>
#include <string>
#include <unordered_map>

namespace jit {
    struct BasicBlock;
}

namespace kernel {
    class Kernel;
    class Thread;
}

namespace x64 {
    struct BasicBlock;
}

namespace emulator {

    class BasicBlock {
    public:
        explicit BasicBlock(x64::BasicBlock cpuBasicBlock);
        ~BasicBlock();

        const x64::BasicBlock& basicBlock() const { return cpuBasicBlock_; }

        u64 start() const;
        u64 end() const;

        BasicBlock* findNext(u64 address);

        void addSuccessor(BasicBlock* other);
        void removeFromCaches();

        size_t size() const;
        void onCall();

    private:
        void removePredecessor(BasicBlock* other);
        void removeSucessor(BasicBlock* other);

        static constexpr size_t CACHE_SIZE = 2;
        x64::BasicBlock cpuBasicBlock_;
        std::array<BasicBlock*, CACHE_SIZE> next_;
        std::array<u64, CACHE_SIZE> nextCount_;
        std::unordered_set<BasicBlock*> successors_;
        std::unordered_set<BasicBlock*> predecessors_;
        u64 calls_ { 0 };

        std::optional<x64::NativeBasicBlock> jitBasicBlock_;
        bool compilationAttempted_ { false };
    };

    class VM {
    public:
        explicit VM(x64::Cpu& cpu, x64::Mmu& mmu, kernel::Kernel& kernel);
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
            void on_munmap(u64 base, u64 length, BitFlags<x64::PROT> prot) override;
        private:
            x64::Mmu* mmu_ { nullptr };
            VM* vm_ { nullptr };
        };

        class CpuCallback : public x64::Cpu::Callback {
        public:
            CpuCallback(x64::Cpu* cpu, VM* vm);
            ~CpuCallback();
            void onSyscall() override;
            void onCall(u64 address) override;
            void onRet() override;
        private:
            x64::Cpu* cpu_ { nullptr };
            VM* vm_ { nullptr };
        };

    private:
        void log(size_t ticks, const x64::X64Instruction& instruction) const;

        BasicBlock* fetchBasicBlock();

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

        x64::Cpu& cpu_;
        x64::Mmu& mmu_;
        kernel::Kernel& kernel_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;

        kernel::Thread* currentThread_ { nullptr };

        std::vector<std::unique_ptr<BasicBlock>> basicBlocks_;
        std::unordered_map<u64, BasicBlock*> basicBlocksByAddress_;

#ifdef VM_BASICBLOCK_TELEMETRY
        u64 blockCacheHits_ { 0 };
        u64 blockCacheMisses_ { 0 };
        u64 mapAccesses_ { 0 };
        u64 mapHit_ { 0 };
        u64 mapMiss_ { 0 };
        std::unordered_map<u64, u64> basicBlockCount_;
        std::unordered_map<u64, u64> basicBlockCacheMissCount_;
#endif

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;
    };

}

#endif