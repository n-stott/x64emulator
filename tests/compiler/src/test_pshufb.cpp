#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testPshufb(MMX dst, MMX src) {
    Assembler assembler;
    assembler.pshufb(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PSHUFB_MMX_MMXM64);
    MMX disdst = ins.op0<MMX>();
    verify(disdst == dst);
    MMXM64 dissrc = ins.op1<MMXM64>();
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testPshufb() {
    std::array<MMX, 8> regs {{
        MMX::MM0,
        MMX::MM1,
        MMX::MM2,
        MMX::MM3,
        MMX::MM4,
        MMX::MM5,
        MMX::MM6,
        MMX::MM7,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            testPshufb(dst, src);
        }
    }
}

int main() {
    try {
        testPshufb();
    } catch(...) {
        return 1;
    }
}