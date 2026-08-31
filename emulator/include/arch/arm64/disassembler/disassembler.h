#ifndef ARM64DISASSEMBLER_H
#define ARM64DISASSEMBLER_H

#include "arch/arm64/instructions/instruction.h"
#include "arch/arm64/types.h"
#include <vector>

namespace arm64 {

    class Disassembler {
    public:
        virtual ~Disassembler() = default;

        struct DisassemblyResult {
            std::vector<Instruction> instructions;
            const u8* next;
            size_t remainingSize;
            u64 nextAddress;
        };

        virtual DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address) = 0;
    };
}

#endif