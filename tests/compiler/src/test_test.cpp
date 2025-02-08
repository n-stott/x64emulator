#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testTest64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.test(dst, src);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
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
        testTest64();
    } catch(...) {
        return 1;
    }
}