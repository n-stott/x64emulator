#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testMov32(R32 dst, R32 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R32_R32);
    R32 disdst = ins.op0<R32>();
    R32 dissrc = ins.op1<R32>();
    verify(disdst == dst);
    verify(dissrc == src);
}

void testMov32() {
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
            testMov32(dst, src);
        }
    }
}

void testMov64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R64_R64);
    verify(ins.op0<R64>() == dst);
    verify(ins.op1<R64>() == src);
}

void testMov64() {
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
            testMov64(dst, src);
        }
    }
}

int main() {
    try {
        testMov32();
        testMov64();
    } catch(...) {
        return 1;
    }
}