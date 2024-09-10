#ifndef CAPSTONEWRAPPER_H
#define CAPSTONEWRAPPER_H

#include "instructions/x64instruction.h"
#include "types.h"
#include <vector>

namespace x64 {

    class CapstoneWrapper {
    public:
        struct DisassemblyResult {
            std::vector<X64Instruction> instructions;
            const u8* next;
            size_t remainingSize;
            u64 nextAddress;
        };

        static DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address);
    };
}

#endif