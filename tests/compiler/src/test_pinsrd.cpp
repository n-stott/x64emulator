#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testPextrd(R32 dst, XMM src, u8 imm) {
    Assembler assembler;
    assembler.pextrd(dst, src, imm);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PEXTRD_RM32_XMM_IMM);
    RM32 disdst = ins.op0<RM32>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    XMM dissrc = ins.op1<XMM>();
    verify(dissrc == src);
    Imm disimm = ins.op2<Imm>();
    verify(disimm.as<u8>() == imm);
}

void testPextrd() {
    std::array<R32, 16> reg32 {{
        R32::EAX,
        R32::EBX,
        R32::ECX,
        R32::EDX,
        R32::EDI,
        R32::ESI,
        R32::ESP,
        R32::EBP,
        R32::R8D,
        R32::R9D,
        R32::R10D,
        R32::R11D,
        R32::R12D,
        R32::R13D,
        R32::R14D,
        R32::R15D,
    }};
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
    for(auto dst : reg32) {
        for(auto src : regs) {
            for(u8 imm = 0; imm < 4; ++imm) {
                testPextrd(dst, src, imm);
            }
        }
    }
}

int main() {
    try {
        testPextrd();
    } catch(...) {
        return 1;
    }
}