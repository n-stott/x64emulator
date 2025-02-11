#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testMov16(R16 dst, R16 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R16_R16);
    R16 disdst = ins.op0<R16>();
    R16 dissrc = ins.op1<R16>();
    verify(disdst == dst);
    verify(dissrc == src);
}

void testMov16() {
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
            testMov16(dst, src);
        }
    }
}

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