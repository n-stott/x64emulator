#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testRoundps(XMM dst, XMM src, u8 imm) {
    Assembler assembler;
    assembler.roundps(dst, src, imm);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::ROUNDPS_XMM_XMM_IMM);
    XMM disdst = ins.op0<XMM>();
    verify(disdst == dst);
    XMM dissrc = ins.op1<XMM>();
    verify(dissrc == src);
    Imm disimm = ins.op2<Imm>();
    verify(disimm.as<u8>() == imm);
}

void testRoundps() {
    std::array<XMM, 16> regs {{
        XMM::XMM0,
        XMM::XMM1,
        XMM::XMM2,
        XMM::XMM3,
        XMM::XMM4,
        XMM::XMM5,
        XMM::XMM6,
        XMM::XMM7,
        XMM::XMM8,
        XMM::XMM9,
        XMM::XMM10,
        XMM::XMM11,
        XMM::XMM12,
        XMM::XMM13,
        XMM::XMM14,
        XMM::XMM15,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            for(u16 imm = 0; imm < 0xFF; ++imm) {
                testRoundps(dst, src, (u8)imm);
            }
        }
    }
}

int main() {
    try {
        testRoundps();
    } catch(...) {
        return 1;
    }
}