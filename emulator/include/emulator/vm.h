#ifndef VM_H
#define VM_H

#include "emulator/symbolprovider.h"
#include "emulator/executablememoryallocator.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "utils.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

    class VM;

    class BasicBlock {
    public:
        explicit BasicBlock(x64::BasicBlock cpuBasicBlock);

        const x64::BasicBlock& basicBlock() const { return cpuBasicBlock_; }
        const u8* nativeBasicBlock() const { return nativeBasicBlock_.ptr; }

        u64 start() const;
        u64 end() const;

        BasicBlock* findNext(u64 address);

        void addSuccessor(BasicBlock* other);
        void removeFromCaches();
        void freeNativeBlock(VM&);

        size_t size() const;
        void onCall(VM&);
        void tryCompile(VM&, u64 callLimit);
        void tryPatch();

        u64 calls() const { return calls_; }
        bool hasPendingPatches() const { return !!pendingPatches_; }

    private:
        void removePredecessor(BasicBlock* other);
        void removeSucessor(BasicBlock* other);

        x64::BasicBlock cpuBasicBlock_;

        struct FixedDestinationInfo {
            static constexpr size_t CACHE_SIZE = 2;
            std::array<BasicBlock*, CACHE_SIZE> next;
            std::array<u64, CACHE_SIZE> nextCount;

            BasicBlock* findNext(u64 address);
            void addSuccessor(BasicBlock* other);
            void removeSuccessor(BasicBlock* other);
        } fixedDestinationInfo_;

        struct VariableDestinationInfo {
            x64::BlockLookupTable table;
            std::vector<BasicBlock*> next;
            std::vector<u64> nextStart;
            std::vector<u64> nextCount;

            void addSuccessor(BasicBlock* other);
            void removeSuccessor(BasicBlock* other);
        } variableDestinationInfo_;
        
        MemoryBlock nativeBasicBlock_;
        bool compilationAttempted_ { false };

        bool endsWithFixedDestinationJump_ { false };
        std::unordered_map<u64, BasicBlock*> successors_;
        std::unordered_map<u64, BasicBlock*> predecessors_;
        u64 calls_ { 0 };

        struct PendingPatches {
            std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
            std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        };
        std::optional<PendingPatches> pendingPatches_;

        friend class BasicBlockTest;
    };

    class BasicBlockTest {
        static_assert(sizeof(BasicBlock::cpuBasicBlock_) == 0x18);
        static_assert(sizeof(BasicBlock::fixedDestinationInfo_) == 0x20);

        static_assert(offsetof(BasicBlock, variableDestinationInfo_) == x64::BLOCK_LOOKUP_TABLE_OFFSET);
        static_assert(offsetof(BasicBlock::VariableDestinationInfo, table) == 0);

        static_assert(sizeof(BasicBlock::variableDestinationInfo_) == 0x68);

        static_assert(offsetof(BasicBlock, nativeBasicBlock_) == x64::NATIVE_BLOCK_OFFSET);
        static_assert(offsetof(MemoryBlock, ptr) == 0);
    };

    class VM {
    public:
        explicit VM(x64::Cpu& cpu, x64::Mmu& mmu);
        ~VM();

        void setEnableJit(bool enable);
        bool jitEnabled() const { return jitEnabled_; }

        void setEnableJitChaining(bool enable);
        bool jitChainingEnabled() const { return jitChainingEnabled_; }

        void setOptimizationLevel(int level);
        int optimizationLevel() const { return optimizationLevel_; }

        void crash();
        bool hasCrashed() const { return hasCrashed_; }

        void contextSwitch(kernel::Thread* newThread);

        void execute(kernel::Thread* thread);

        void push64(u64 value);

        void tryCreateJitTrampoline();
        MemoryBlock tryMakeNative(const u8* code, size_t size);
        void freeNative(MemoryBlock);

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

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;

        kernel::Thread* currentThread_ { nullptr };

        std::vector<std::unique_ptr<BasicBlock>> basicBlocks_;
        std::unordered_map<u64, BasicBlock*> basicBlocksByAddress_;
        u64 jitExits_ { 0 };
        u64 jitExitRet_ { 0 };
        std::unordered_set<u64> distinctJitExitRet_;
        u64 jitExitCallRM64_ { 0 };
        std::unordered_set<u64> distinctJitExitCallRM64_;
        u64 jitExitJmpRM64_ { 0 };
        std::unordered_set<u64> distinctJitExitJmpRM64_;
        u64 avoidableExits_ { 0 };

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

        ExecutableMemoryAllocator allocator_;
        std::optional<MemoryBlock> jitTrampoline_;
        bool jitTrampolineCompilationAttempted_ { false };

        bool jitEnabled_ { false };
        bool jitChainingEnabled_ { false };
        int optimizationLevel_ { 0 };
    };

}

#endif