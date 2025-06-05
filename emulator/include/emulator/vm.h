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
    class CapstoneWrapper;
    class Compiler;
}

namespace emulator {

    class VM;
    class CompilationQueue;
    class BasicBlock;

    template<typename T>
    class Optional {
        static_assert(std::is_default_constructible_v<T>);
    public:
        static constexpr size_t VALUE_OFFSET = 0;

        operator bool() const {
            return present_;
        }

        T* ptr() {
            if(!present_) return nullptr;
            return &value_;
        }

        T* operator->() {
            if(!present_) return nullptr;
            return &value_;
        }

        const T* operator->() const {
            if(!present_) return nullptr;
            return &value_;
        }

        void reset() {
            present_ = false;
            value_.~T();
            new(&value_)T();
        }

        void emplace() {
            value_.~T();
            new(&value_)T();
            present_ = true;
        }
        
    private:
        T value_ {};
        bool present_ { false };
        friend class OptionalTests;
    };

    struct OptionalTests {
        static_assert(offsetof(Optional<u32>, value_) == Optional<u32>::VALUE_OFFSET);
        static_assert(offsetof(Optional<u64>, value_) == Optional<u64>::VALUE_OFFSET);
    };

    class JitBasicBlock {
        friend class BasicBlockTest;
    public:
        static void tryCreateInplace(Optional<JitBasicBlock>* dst, const x64::BasicBlock& bb, BasicBlock* currentBb, x64::Compiler* compiler, int optimizationLevel, ExecutableMemoryAllocator* allocator);

        JitBasicBlock();
        ~JitBasicBlock();

        bool needsPatching() const {
            return !!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock
                || !!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock;
        }

        const u8* executableMemory() const {
            return executableMemory_.ptr;
        }

        u8* mutableExecutableMemory() {
            return executableMemory_.ptr;
        }

        void syncBlockLookupTable(u64 size, const u64* addresses, const BasicBlock** blocks, u64* hitCounts);

        void tryPatch(std::optional<size_t>* pendingPatch, const BasicBlock* next, x64::Compiler* compiler);

        template<typename Functor>
        void forAllPendingPatches(bool continuing, Functor&& functor) {
            if(continuing && !!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock) {
                functor(&pendingPatches_.offsetOfReplaceableJumpToContinuingBlock);
            }
            if(!continuing && !!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock) {
                functor(&pendingPatches_.offsetOfReplaceableJumpToConditionalBlock);
            }
        }

    private:
        JitBasicBlock(const JitBasicBlock&) = delete;
        JitBasicBlock(JitBasicBlock&&) = delete;

        void setExecutableMemory(MemoryBlock executableMemory) {
            executableMemory_ = executableMemory;
        }

        void setPendingPatchToConditionalBlock(size_t offset) {
            assert(!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock);
            pendingPatches_.offsetOfReplaceableJumpToConditionalBlock = offset;
        }

        void setPendingPatchToContinuingBlock(size_t offset) {
            assert(!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock);
            pendingPatches_.offsetOfReplaceableJumpToContinuingBlock = offset;
        }

        MemoryBlock executableMemory_;

        x64::BlockLookupTable variableDestinationTable_;

        u64 calls_ { 0 };

        struct PendingPatches {
            std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
            std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        } pendingPatches_;
    };

    class BasicBlock {
    public:
        explicit BasicBlock(x64::BasicBlock cpuBasicBlock);

        const x64::BasicBlock& basicBlock() const {
            return cpuBasicBlock_;
        }

        JitBasicBlock* jitBasicBlock() {
            if(!jitBasicBlock_) return nullptr;
            return jitBasicBlock_.ptr();
        }

        const u8* nativeCode() const {
            if(!jitBasicBlock_) return nullptr;
            return jitBasicBlock_->executableMemory();
        }

        u64 start() const;
        u64 end() const;

        BasicBlock* findNext(u64 address);

        void addSuccessor(BasicBlock* other);
        void removeFromCaches();

        size_t size() const;
        void onCall(VM&);
        void tryCompile(VM&, CompilationQueue&);
        void tryPatch(VM&);

        u64 calls() const { return calls_; }

    private:
        void removePredecessor(BasicBlock* other);
        void removeSucessor(BasicBlock* other);

        x64::BasicBlock cpuBasicBlock_;
        Optional<JitBasicBlock> jitBasicBlock_;

        struct FixedDestinationInfo {
            static constexpr size_t CACHE_SIZE = 2;
            std::array<BasicBlock*, CACHE_SIZE> next;
            std::array<u64, CACHE_SIZE> nextCount;

            BasicBlock* findNext(u64 address);
            void addSuccessor(BasicBlock* other);
            void removeSuccessor(BasicBlock* other);
        } fixedDestinationInfo_;

        struct VariableDestinationInfo {
            std::vector<BasicBlock*> next;
            std::vector<u64> nextStart;
            std::vector<u64> nextCount;

            void addSuccessor(BasicBlock* other);
            void removeSuccessor(BasicBlock* other);
        } variableDestinationInfo_;

        void syncBlockLookupTable();
        
        u8* mutableNativeBasicBlock() {
            if(!!jitBasicBlock_) return nullptr;
            return jitBasicBlock_->mutableExecutableMemory();
        }
        
        bool compilationAttempted_ { false };

        u64 calls_ { 0 };
        u64 callsForCompilation_ { 0 };

        bool endsWithFixedDestinationJump_ { false };
        std::unordered_map<u64, BasicBlock*> successors_;
        std::unordered_map<u64, BasicBlock*> predecessors_;

        friend class BasicBlockTest;
    };

    class BasicBlockTest {
        static_assert(sizeof(BasicBlock::cpuBasicBlock_) == 0x18);
        static_assert(sizeof(BasicBlock::fixedDestinationInfo_) == 0x20);
        static_assert(sizeof(BasicBlock::variableDestinationInfo_) == 0x48);

        static_assert(offsetof(BasicBlock, jitBasicBlock_) == 0x18);
        static_assert(Optional<JitBasicBlock>::VALUE_OFFSET == 0);
        
        static_assert(offsetof(BasicBlock, jitBasicBlock_) + offsetof(JitBasicBlock, executableMemory_) == x64::NATIVE_BLOCK_OFFSET);
        static_assert(offsetof(MemoryBlock, ptr) == 0);
        static_assert(offsetof(BasicBlock, jitBasicBlock_) + offsetof(JitBasicBlock, variableDestinationTable_) == x64::BLOCK_LOOKUP_TABLE_OFFSET);
        static_assert(offsetof(BasicBlock, jitBasicBlock_) + offsetof(JitBasicBlock, calls_) == x64::CALLS_OFFSET);
    };


    class CompilationQueue {
    public:
        void process(VM& vm, BasicBlock* bb) {
            queue_.clear();
            queue_.push_back(bb);
            while(!queue_.empty()) {
                bb = queue_.front();
                queue_.pop_front();
                bb->tryCompile(vm, *this);
            }
        }

        void push(BasicBlock* bb) {
            queue_.push_back(bb);
        }

    private:
        std::deque<BasicBlock*> queue_;
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

        ExecutableMemoryAllocator* allocator() { return &allocator_; }

        void crash();
        bool hasCrashed() const { return hasCrashed_; }

        void contextSwitch(kernel::Thread* newThread);

        void execute(kernel::Thread* thread);

        void push64(u64 value);

        void tryCreateJitTrampoline();
        MemoryBlock tryMakeNative(const u8* code, size_t size);
        void freeNative(MemoryBlock);
        x64::Compiler* compiler() const { return compiler_.get(); }
        CompilationQueue& compilationQueue() { return compilationQueue_; }

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

        std::unique_ptr<x64::CapstoneWrapper> disassembler_;
        std::unique_ptr<x64::Compiler> compiler_;
        CompilationQueue compilationQueue_;
    };

}

#endif