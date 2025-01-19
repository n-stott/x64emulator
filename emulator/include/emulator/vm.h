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
            void on_munmap(u64 base, u64 length) override;
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

        x64::Cpu& cpu_;
        x64::Mmu& mmu_;
        kernel::Kernel& kernel_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;

        kernel::Thread* currentThread_ { nullptr };

        class BBlock {
        public:
            explicit BBlock(x64::Cpu::BasicBlock cpuBasicBlock) : cpuBasicBlock_(std::move(cpuBasicBlock)) {
                std::fill(next_.begin(), next_.end(), nullptr);
                std::fill(nextCount_.begin(), nextCount_.end(), 0);
            }

            const x64::Cpu::BasicBlock& basicBlock() const {
                return cpuBasicBlock_;
            }

            u64 start() const {
                verify(!cpuBasicBlock_.instructions.empty(), "Basic block is empty");
                return cpuBasicBlock_.instructions[0].first.address();
            }
            u64 end() const {
                verify(!cpuBasicBlock_.instructions.empty(), "Basic block is empty");
                return cpuBasicBlock_.instructions.back().first.nextAddress();
            }

            BBlock* findNext(u64 address) {
                for(size_t i = 0; i < next_.size(); ++i) {
                    if(!next_[i]) return nullptr;
                    if(next_[i]->start() != address) continue;
                    BBlock* result = next_[i];
                    ++nextCount_[i];
                    if(i > 0 && nextCount_[i] > nextCount_[i-1]) {
                        std::swap(next_[i], next_[i-1]);
                        std::swap(nextCount_[i], nextCount_[i-1]);
                    }
    #ifdef VM_BASICBLOCK_TELEMETRY
                    ++blockCacheHits_;
    #endif
                    return result;
                }
                return nullptr;
            }

            void addSuccessor(BBlock* other) {
                size_t firstAvailableSlot = BBlock::CACHE_SIZE-1;
                for(size_t i = 0; i < next_.size(); ++i) {
                    if(!next_[i]) {
                        firstAvailableSlot = i;
                        break;
                    }
                }
                next_[firstAvailableSlot] = other;
                nextCount_[firstAvailableSlot] = 1;
                successors_.push_back(other);
                other->predecessors_.push_back(this);
            }

            void removePredecessor(BBlock* other) {
                predecessors_.erase(std::remove(predecessors_.begin(), predecessors_.end(), other), predecessors_.end());
            }

            void removeSucessor(BBlock* other) {
                for(size_t i = 0; i < next_.size(); ++i) {
                    const auto* bb1 = next_[i];
                    if(bb1 == other) {
                        next_[i] = nullptr;
                        nextCount_[i] = 0;
                    }
                }
                successors_.erase(std::remove(successors_.begin(), successors_.end(), other), successors_.end());
            }

            void removeFromCaches() {
                for(auto* prev : predecessors_) prev->removeSucessor(this);
                predecessors_.clear();
                for(BBlock* successor : successors_) successor->removePredecessor(this);
                successors_.clear();
            }

        private:
            static constexpr size_t CACHE_SIZE = 3;
            x64::Cpu::BasicBlock cpuBasicBlock_;
            std::array<BBlock*, CACHE_SIZE> next_;
            std::array<u64, CACHE_SIZE> nextCount_;
            std::vector<BBlock*> successors_;
            std::vector<BBlock*> predecessors_;
        };

        std::vector<std::unique_ptr<BBlock>> basicBlocks_;
        std::unordered_map<u64, BBlock*> basicBlocksByAddress_;

#ifdef VM_BASICBLOCK_TELEMETRY
        u64 blockCacheHits_ { 0 };
        u64 blockCacheMisses_ { 0 };
        u64 mapAccesses_ { 0 };
        std::unordered_map<u64, u64> basicBlockCount_;
        std::unordered_map<u64, u64> basicBlockCacheMissCount_;
#endif

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;
    };

}

#endif