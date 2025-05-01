#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testCmpps(XMM dst, XMM src, FCond cond) {
    Assembler assembler;
    assembler.cmpps(dst, src, (u8)cond);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::CMPPS_XMM_XMMM128);
    XMM disdst = ins.op0<XMM>();
    verify(disdst == dst);
    XMMM128 dissrc = ins.op1<XMMM128>();
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
    FCond discond = ins.op2<FCond>();
    verify(discond == cond);
}

void testCmpps() {
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
    std::array<FCond, 8> conds {{
        FCond::EQ,
        FCond::LT,
        FCond::LE,
        FCond::UNORD,
        FCond::NEQ,
        FCond::NLT,
        FCond::NLE,
        FCond::ORD,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            for(FCond cond : conds) {
                testCmpps(dst, src, cond);
            }
        }
    }
}

int main() {
    try {
        testCmpps();
    } catch(...) {
        return 1;
    }
}