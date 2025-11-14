#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testPmovmskb(R32 dst, MMX src) {
    Assembler assembler;
    assembler.pmovmskb(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PMOVMSKB_R32_MMX);
    R32 disdst = ins.op0<R32>();
    MMX dissrc = ins.op1<MMX>();
    verify(disdst == dst);
    verify(dissrc == src);
}

void testPmovmskb() {
    std::array<R32, 16> regs {{
        R32::EAX,
        R32::ECX,
        R32::EDX,
        R32::EBX,
        R32::ESP,
        R32::EBP,
        R32::ESI,
        R32::EDI,
        R32::R8D,
        R32::R9D,
        R32::R10D,
        R32::R11D,
        R32::R12D,
        R32::R13D,
        R32::R14D,
        R32::R15D,
    }};
    std::array<MMX, 8> mmxs {{
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
        for(auto src : mmxs) {
            testPmovmskb(dst, src);
        }
    }
}

int main() {
    try {
        testPmovmskb();
    } catch(...) {
        return 1;
    }
}