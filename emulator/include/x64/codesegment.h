#ifndef CODESEGMENT_H
#define CODESEGMENT_H

#include "x64/compiler/jit.h"
#include "x64/instructions/basicblock.h"

namespace x64 {

    class CompilationQueue;

    class CodeSegment {
    public:
        explicit CodeSegment(BasicBlock cpuBasicBlock);

        const BasicBlock& basicBlock() const {
            return cpuBasicBlock_;
        }

        const JitBasicBlock* jitBasicBlock() const {
            return jitBasicBlock_;
        }

        JitBasicBlock* jitBasicBlock() {
            return jitBasicBlock_;
        }

        u64 start() const;
        u64 end() const;

        CodeSegment* findNext(u64 address);

        void addSuccessor(CodeSegment* other);
        void removeFromCaches();

        size_t size() const;
        void onCall(Jit* jit, CompilationQueue& compilationQueue);
        void onCpuCall();
        void onJitCall();
        void tryCompile(Jit&, CompilationQueue&);
        void tryPatch(Jit&);

        u64 calls() const { return calls_ + (!!jitBasicBlock_ ? jitBasicBlock_->calls() : 0); }

        void dumpGraphviz(std::ostream&, std::unordered_map<void*, u32>& counter) const;

    private:
        void removePredecessor(CodeSegment* other);
        void removeSucessor(CodeSegment* other);

        BasicBlock cpuBasicBlock_;
        JitBasicBlock* jitBasicBlock_ { nullptr };

        struct FixedDestinationInfo {
            static constexpr size_t CACHE_SIZE = 2;
            std::array<CodeSegment*, CACHE_SIZE> next;
            std::array<u64, CACHE_SIZE> nextCount;

            CodeSegment* findNext(u64 address);
            void addSuccessor(CodeSegment* other);
            void removeSuccessor(CodeSegment* other);
        } fixedDestinationInfo_;

        struct VariableDestinationInfo {
            std::vector<CodeSegment*> next;
            std::vector<JitBasicBlock*> nextJit;
            std::vector<u64> nextStart;
            std::vector<u64> nextCount;

            void addSuccessor(CodeSegment* other);
            void removeSuccessor(CodeSegment* other);
        } variableDestinationInfo_;

        void syncBlockLookupTable();
        
        bool compilationAttempted_ { false };

        u64 calls_ { 0 };
        u64 callsForCompilation_ { 0 };

        bool endsWithFixedDestinationJump_ { false };
        std::unordered_map<u64, CodeSegment*> successors_;
        std::unordered_map<u64, CodeSegment*> predecessors_;

        friend class CodeSegmentTest;
    };

    class CodeSegmentTest {
        static_assert(sizeof(CodeSegment::cpuBasicBlock_) == 0x20);
        static_assert(sizeof(CodeSegment::fixedDestinationInfo_) == 0x20);
        static_assert(sizeof(CodeSegment::variableDestinationInfo_) == 0x60);

        static_assert(offsetof(CodeSegment, jitBasicBlock_) == 0x20);
    };

    class CompilationQueue {
    public:
        void process(x64::Jit& jit, x64::CodeSegment* seg) {
            queue_.clear();
            queue_.push_back(seg);
            while(!queue_.empty()) {
                seg = queue_.back();
                queue_.pop_back();
                seg->tryCompile(jit, *this);
            }
        }

        void push(x64::CodeSegment* seg) {
            queue_.push_back(seg);
        }

    private:
        std::vector<x64::CodeSegment*> queue_;
    };

}

#endif