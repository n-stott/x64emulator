#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testTest16(R16 dst, R16 src) {
    Assembler assembler;
    assembler.test(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::TEST_RM16_R16);
    RM16 disdst = ins.op0<RM16>();
    R16 dissrc = ins.op1<R16>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc == src);
}

void testTest16() {
    std::array<R16, 16> regs {{
        R16::AX,
        R16::CX,
        R16::DX,
        R16::BX,
        R16::SP,
        R16::BP,
        R16::SI,
        R16::DI,
        R16::R8W,
        R16::R9W,
        R16::R10W,
        R16::R11W,
        R16::R12W,
        R16::R13W,
        R16::R14W,
        R16::R15W,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            testTest16(dst, src);
        }
    }
}

void testTest64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.test(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::TEST_RM64_R64);
    RM64 disdst = ins.op0<RM64>();
    R64 dissrc = ins.op1<R64>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc == src);
}

void testTest64() {
    std::array<R64, 16> regs {{
        R64::RAX,
        R64::RCX,
        R64::RDX,
        R64::RBX,
        R64::RSP,
        R64::RBP,
        R64::RSI,
        R64::RDI,
        R64::R8,
        R64::R9,
        R64::R10,
        R64::R11,
        R64::R12,
        R64::R13,
        R64::R14,
        R64::R15,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            testTest64(dst, src);
        }
    }
}

int main() {
    try {
        testTest16();
        testTest64();
    } catch(...) {
        return 1;
    }
}