#include "arch/arm64/instructions/instruction.h"
#include <fmt/core.h>

namespace arm64 {

#ifndef NDEBUG
    std::atomic<u8> Instruction::operandTypeId_ { 0 };
#endif

    bool Instruction::isBranch() const {
        return insn_ == Insn::UNKNOWN
            || false;
    }

    std::string Instruction::toString() const {
        return "Instruction::toString missing";
    }

}