#ifndef ARM64ZYDISWRAPPER_H
#define ARM64ZYDISWRAPPER_H

#include "arch/arm64/disassembler/disassembler.h"
#include "arch/arm64/instructions/instruction.h"
#include "arch/arm64/types.h"
#include <vector>

namespace arm64 {

    class CapstoneWrapper : public Disassembler {
    public:

        DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address) override;

    private:
        std::vector<Instruction> instructions_;
    };
}

#endif