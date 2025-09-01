#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "x64/instructions/x64instruction.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace x64 {

    class Cpu;

    using CpuExecPtr = void(*)(Cpu&, const X64Instruction&);

    class BasicBlock {
    public:
        BasicBlock(std::vector<std::pair<X64Instruction, CpuExecPtr>> instructions)
                : instructions_(std::move(instructions)) {
            assert(!instructions_.empty());
            endsWithFixedDestinationJump_ = instructions_.back().first.isFixedDestinationJump();
            hasAtomic_ = std::any_of(instructions_.begin(), instructions_.end(), [](const auto& p) {
                return p.first.lock();
            });
        }

        const std::vector<std::pair<X64Instruction, CpuExecPtr>>& instructions() const {
            return instructions_;
        }

        bool endsWithFixedDestinationJump() const {
            return endsWithFixedDestinationJump_;
        }

        bool hasAtomicInstruction() const {
            return hasAtomic_;
        }

    private:
        std::vector<std::pair<X64Instruction, CpuExecPtr>> instructions_;
        bool endsWithFixedDestinationJump_ { false };
        bool hasAtomic_ { false };
    };

    struct NativeBasicBlock {
        std::vector<u8> nativecode;
        std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
        std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        std::optional<size_t> offsetOfReplaceableCallstackPush;
        std::optional<size_t> offsetOfReplaceableCallstackPop;
    };

}

#endif
