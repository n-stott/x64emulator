#ifndef VM_H
#define VM_H

#include "emulator/symbolprovider.h"
#include "emulator/vmthread.h"
#include "x64/compiler/jit.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "intervalvector.h"
#include "utils.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x64 {
    class BasicBlock;
    class Compiler;
    class Disassembler;
    class JitBasicBlock;
    class Jit;
}

namespace emulator {

    class VM;
    class CompilationQueue;

    class BasicBlock {
    public:
        explicit BasicBlock(x64::BasicBlock cpuBasicBlock);

        const x64::BasicBlock& basicBlock() const {
            return cpuBasicBlock_;
        }

        const x64::JitBasicBlock* jitBasicBlock() const {
            return jitBasicBlock_;
        }

        x64::JitBasicBlock* jitBasicBlock() {
            return jitBasicBlock_;
        }

        u64 start() const;
        u64 end() const;

        BasicBlock* findNext(u64 address);

        void addSuccessor(BasicBlock* other);
        void removeFromCaches();

        size_t size() const;
        void onCall(VM&);
        void onCpuCall();
        void onJitCall();
        void tryCompile(x64::Jit&, CompilationQueue&, int optimizationLevel);
        void tryPatch(x64::Jit&);

        u64 calls() const { return calls_ + (!!jitBasicBlock_ ? jitBasicBlock_->calls() : 0); }

    private:
        void removePredecessor(BasicBlock* other);
        void removeSucessor(BasicBlock* other);

        x64::BasicBlock cpuBasicBlock_;
        x64::JitBasicBlock* jitBasicBlock_ { nullptr };

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
            std::vector<x64::JitBasicBlock*> nextJit;
            std::vector<u64> nextStart;
            std::vector<u64> nextCount;

            void addSuccessor(BasicBlock* other);
            void removeSuccessor(BasicBlock* other);
        } variableDestinationInfo_;

        void syncBlockLookupTable();
        
        bool compilationAttempted_ { false };

        u64 calls_ { 0 };
        u64 callsForCompilation_ { 0 };

        bool endsWithFixedDestinationJump_ { false };
        std::unordered_map<u64, BasicBlock*> successors_;
        std::unordered_map<u64, BasicBlock*> predecessors_;

        friend class BasicBlockTest;
    };

    class BasicBlockTest {
        static_assert(sizeof(BasicBlock::cpuBasicBlock_) == 0x20);
        static_assert(sizeof(BasicBlock::fixedDestinationInfo_) == 0x20);
        static_assert(sizeof(BasicBlock::variableDestinationInfo_) == 0x60);

        static_assert(offsetof(BasicBlock, jitBasicBlock_) == 0x20);
    };

    class CompilationQueue {
    public:
        void process(x64::Jit& jit, emulator::BasicBlock* bb, int optimizationLevel) {
            queue_.clear();
            queue_.push_back(bb);
            while(!queue_.empty()) {
                bb = queue_.back();
                queue_.pop_back();
                bb->tryCompile(jit, *this, optimizationLevel);
            }
        }

        void push(emulator::BasicBlock* bb) {
            queue_.push_back(bb);
        }

    private:
        std::vector<emulator::BasicBlock*> queue_;
    };

    class VM {
    public:
        explicit VM(x64::Cpu& cpu, x64::Mmu& mmu);
        ~VM();

        void setEnableJit(bool enable);
        bool jitEnabled() const { return jitEnabled_; }

        void setEnableJitChaining(bool enable);
        bool jitChainingEnabled() const;

        void setEnableJitStats(bool enable);
        bool jitStatsEnabled() const { return jitStatsEnabled_; }

        void setOptimizationLevel(int level);
        int optimizationLevel() const { return optimizationLevel_; }

        void setDisassembler(int disassembler);

        x64::Jit* jit() { return jit_.get(); }
        CompilationQueue& compilationQueue() { return compilationQueue_; }

        void execute(VMThread* thread);

        void tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) const;

        class MmuCallback : public x64::Mmu::Callback {
        public:
            MmuCallback(x64::Mmu* mmu, VM* vm);
            ~MmuCallback();
            void onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) override;
            void onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) override;
            void onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) override;
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

        void contextSwitch(VMThread* newThread);
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

        void handleRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot);
        void handleRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter);
        void handleRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot);

        void dumpJitTelemetry(const std::vector<const BasicBlock*>& blocks) const;

        x64::Cpu& cpu_;
        x64::Mmu& mmu_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;

        VMThread* currentThread_ { nullptr };

        IntervalVector<BasicBlock> basicBlocks_;
        std::unordered_map<u64, BasicBlock*> basicBlocksByAddress_;

        u64 jitExits_ { 0 };
        u64 avoidableExits_ { 0 };
#ifdef VM_JIT_TELEMETRY
        u64 jitExitRet_ { 0 };
        std::unordered_set<u64> distinctJitExitRet_;
        u64 jitExitCallRM64_ { 0 };
        std::unordered_set<u64> distinctJitExitCallRM64_;
        u64 jitExitJmpRM64_ { 0 };
        std::unordered_set<u64> distinctJitExitJmpRM64_;
#endif

#ifdef VM_BASICBLOCK_TELEMETRY
        u64 blockCacheHits_ { 0 };
        u64 blockCacheMisses_ { 0 };
        u64 mapAccesses_ { 0 };
        u64 mapHit_ { 0 };
        u64 mapMiss_ { 0 };
        std::unordered_map<u64, u64> basicBlockCount_;
        std::unordered_map<u64, u64> basicBlockCacheMissCount_;
#endif

        std::vector<x64::X64Instruction> blockInstructions_;

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

        bool jitEnabled_ { false };
        bool jitStatsEnabled_ { false };
        int optimizationLevel_ { 0 };

        std::unique_ptr<x64::Disassembler> disassembler_;
        std::unique_ptr<x64::Jit> jit_;
        CompilationQueue compilationQueue_;
    };

}

#endif