#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "x64/disassembler/zydiswrapper.h"
#include <sys/mman.h>

int main(int, char**) {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(1);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);
    Cpu cpu(mmu);

    std::array<X64Instruction, 1> instructions {{
        // ret
        X64Instruction::make(0x0, Insn::RET, 1),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    Compiler compiler;
    auto nativebb = compiler.tryCompile(bb);
    if(!nativebb) {
        fmt::print(stderr, "failed to compile\n");
        return 1;
    }

    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(nativebb->nativecode.data(), nativebb->nativecode.size(), 0x0);
    fmt::print("{} instructions\n", disassembly.instructions.size());

    return 0;
}