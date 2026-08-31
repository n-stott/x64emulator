#ifndef ARM64BASICBLOCK_H
#define ARM64BASICBLOCK_H

#include "arch/arm64/instructions/instruction.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace arm64 {

    class Cpu;

    using CpuExecPtr = void(*)(Cpu&, const Instruction&);

    class BasicBlock {
    public:
        BasicBlock(std::vector<std::pair<Instruction, CpuExecPtr>> instructions)
                : instructions_(std::move(instructions)) {
            assert(!instructions_.empty());
        }

        const std::vector<std::pair<Instruction, CpuExecPtr>>& instructions() const {
            return instructions_;
        }

        bool endsWithFixedDestinationJump() const { return false; }
        bool endsWithDirectCall() const { return false; }

    private:
        std::vector<std::pair<Instruction, CpuExecPtr>> instructions_;
    };

}

#endif
