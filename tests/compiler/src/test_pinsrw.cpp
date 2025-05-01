#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testPinsrw(XMM dst, R32 src, u8 imm) {
    Assembler assembler;
    assembler.pinsrw(dst, src, imm);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::PINSRW_XMM_R32_IMM);
    XMM disdst = ins.op0<XMM>();
    verify(disdst == dst);
    R32 dissrc = ins.op1<R32>();
    verify(dissrc == src);
    Imm disimm = ins.op2<Imm>();
    verify(disimm.as<u8>() == imm);
}

void testPinsrw() {
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
    std::array<R32, 16> gprs {{
        R32::EAX,
        R32::EBX,
        R32::ECX,
        R32::EDX,
        R32::ESI,
        R32::EDI,
        R32::EBP,
        R32::ESP,
        R32::R8D,
        R32::R9D,
        R32::R10D,
        R32::R11D,
        R32::R12D,
        R32::R13D,
        R32::R14D,
        R32::R15D,
    }};
    std::array<u8, 4> imms {{
        0,
        1,
        2,
        3,
    }};
    for(auto dst : regs) {
        for(auto src : gprs) {
            for(auto imm : imms) {
                testPinsrw(dst, src, imm);
            }
        }
    }
}

int main() {
    try {
        testPinsrw();
    } catch(...) {
        return 1;
    }
}