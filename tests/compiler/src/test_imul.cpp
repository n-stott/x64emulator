#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testImul32(R32 dst, R32 src) {
    Assembler assembler;
    assembler.imul(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::IMUL2_R32_RM32);
    R32 disdst = ins.op0<R32>();
    RM32 dissrc = ins.op1<RM32>();
    verify(dissrc.isReg);
    verify(disdst == dst);
    verify(dissrc.reg == src);
}

void testImul32() {
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
    for(auto dst : regs) {
        for(auto src : regs) {
            testImul32(dst, src);
        }
    }
}

void testImul64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.imul(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::IMUL2_R64_RM64);
    R64 disdst = ins.op0<R64>();
    RM64 dissrc = ins.op1<RM64>();
    verify(disdst == dst);
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testImul64() {
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
            testImul64(dst, src);
        }
    }
}

int main() {
    try {
        testImul32();
        testImul64();
    } catch(...) {
        return 1;
    }
}