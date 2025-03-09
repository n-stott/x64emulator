#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testPshufd(XMM dst, XMM src, u8 imm) {
    Assembler assembler;
    assembler.pshufd(dst, src, imm);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PSHUFD_XMM_XMMM128_IMM);
    XMM disdst = ins.op0<XMM>();
    verify(disdst == dst);
    XMMM128 dissrc = ins.op1<XMMM128>();
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
    Imm disimm = ins.op2<Imm>();
    verify(disimm.as<u8>() == imm);
}

void testPshufd() {
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
            for(u16 imm = 0; imm < 256; ++imm) {
                testPshufd(dst, src, (u8)imm);
            }
        }
    }
}

int main() {
    try {
        testPshufd();
    } catch(...) {
        return 1;
    }
}