#ifndef X64ZYDISWRAPPER_H
#define X64ZYDISWRAPPER_H

#include "arch/x64/disassembler/disassembler.h"
#include "arch/x64/instructions/instruction.h"
#include "arch/x64/types.h"
#include <vector>

namespace x64 {

    class ZydisWrapper : public Disassembler {
    public:

        DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address) override;

    private:
        std::vector<Instruction> instructions_;
    };
}

#endif