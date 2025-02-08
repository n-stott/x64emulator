#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "x64/instructions/x64instruction.h"
#include <vector>

namespace x64 {

    class Cpu;

    using CpuExecPtr = void(*)(Cpu&, const X64Instruction&);
    using NativeExecPtr = void(*)(u64* gprs, u8* memory, u64* rflags);

    struct BasicBlock {
        std::vector<std::pair<X64Instruction, CpuExecPtr>> instructions;

        bool endsWithFixedDestinationJump() const {
            return instructions.back().first.isFixedDestinationJump();
        }
    };

    struct NativeBasicBlock {
        std::vector<u8> nativecode;
    };

}

#endif