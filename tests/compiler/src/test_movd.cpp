#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testMovd32(MMX dst, R32 src) {
    Assembler assembler;
    assembler.movd(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOVD_MMX_RM32);
    MMX disdst = ins.op0<MMX>();
    RM32 dissrc = ins.op1<RM32>();
    verify(disdst == dst);
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testMovd32(R32 dst, MMX src) {
    Assembler assembler;
    assembler.movd(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOVD_RM32_MMX);
    RM32 disdst = ins.op0<RM32>();
    MMX dissrc = ins.op1<MMX>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc == src);
}

void testMovd32() {
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
    std::array<MMX, 16> mmxs {{
        MMX::MM0,
        MMX::MM1,
        MMX::MM2,
        MMX::MM3,
        MMX::MM4,
        MMX::MM5,
        MMX::MM6,
        MMX::MM7,
    }};
    for(auto r32 : regs) {
        for(auto mmx : mmxs) {
            testMovd32(r32, mmx);
            testMovd32(mmx, r32);
        }
    }
}

int main() {
    try {
        testMovd32();
    } catch(...) {
        return 1;
    }
}