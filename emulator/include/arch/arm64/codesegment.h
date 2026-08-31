#ifndef ARM64CODESEGMENT_H
#define ARM64CODESEGMENT_H

#include "arch/arm64/instructions/basicblock.h"
#include "smallmap.h"

namespace arm64 {

    class CompilationQueue;

    class CodeSegment {
    public:
        explicit CodeSegment(BasicBlock cpuBasicBlock);
        ~CodeSegment();

        const BasicBlock& basicBlock() const {
            return cpuBasicBlock_;
        }

        u64 start() const;
        u64 end() const;

        CodeSegment* findNext(u64 address);

        void addSuccessor(CodeSegment* succ);
        void addReturn(CodeSegment* other);
        void removeFromCaches();

        size_t size() const;
        void onCpuCall();

        u64 calls() const { return calls_; }

        void dumpGraphviz(std::ostream&, std::unordered_map<void*, u32>& counter) const;

    private:
        void removePredecessor(CodeSegment* other);
        void removeSucessor(CodeSegment* succ);
        void removeReturn(CodeSegment* ret);
        void removeCallPredecessor(CodeSegment* other);

        BasicBlock cpuBasicBlock_;

        struct FixedDestinationInfo {
            static constexpr size_t CACHE_SIZE = 2;
            std::array<CodeSegment*, CACHE_SIZE> next;
            std::array<u64, CACHE_SIZE> nextCount;

            CodeSegment* findNext(u64 address);
            void addSuccessor(CodeSegment* succ);
            void removeSuccessor(CodeSegment* succ);
        } fixedDestinationInfo_;

        struct VariableDestinationInfo {
            std::vector<CodeSegment*> next;
            std::vector<u64> nextStart;
            std::vector<u64> nextCount;

            void addSuccessor(CodeSegment* succ);
            void removeSuccessor(CodeSegment* succ);
        } variableDestinationInfo_;

        struct ReturnDestinationInfo {
            CodeSegment* ret { nullptr };

            void addReturn(CodeSegment* other);
        } returnDestinationInfo_;
        
        bool compilationAttempted_ { false };

        u64 calls_ { 0 };
        u64 callsForCompilation_ { 0 };

        bool endsWithFixedDestinationJump_ { false };
        SmallMap<u64, CodeSegment*, 2> successors_;
        std::unordered_map<u64, CodeSegment*> predecessors_;
        SmallMap<u64, CodeSegment*, 4> callPredecessors_;

        friend class CodeSegmentTest;
    };

}

#endif