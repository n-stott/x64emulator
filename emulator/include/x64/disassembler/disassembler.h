#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "x64/instructions/x64instruction.h"
#include "x64/types.h"
#include <vector>

namespace x64 {

    class Disassembler {
    public:
        virtual ~Disassembler() = default;

        struct DisassemblyResult {
            std::vector<X64Instruction> instructions;
            const u8* next;
            size_t remainingSize;
            u64 nextAddress;
        };

        virtual DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address) = 0;
    };
}

#endif