#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testPshufb(MMX dst, MMX src) {
    Assembler assembler;
    assembler.pshufb(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
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

void testPshufb128(XMM dst, XMM src) {
    Assembler assembler;
    assembler.pshufb(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PSHUFB_XMM_XMMM128);
    XMM disdst = ins.op0<XMM>();
    verify(disdst == dst);
    XMMM128 dissrc = ins.op1<XMMM128>();
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testPshufb64() {
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

void testPshufb128() {
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
            testPshufb128(dst, src);
        }
    }
}

int main() {
    try {
        testPshufb64();
        testPshufb128();
    } catch(...) {
        return 1;
    }
}