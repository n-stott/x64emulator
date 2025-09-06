#ifndef CAPSTONEWRAPPER_H
#define CAPSTONEWRAPPER_H

#include "x64/disassembler/disassembler.h"
#include "x64/instructions/x64instruction.h"
#include "x64/types.h"
#include <vector>

namespace x64 {

    class CapstoneWrapper : public Disassembler {
    public:

        DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address) override;

    private:
        std::vector<X64Instruction> instructions_;
    };
}

#endif