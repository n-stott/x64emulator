#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testAnd32(R32 dst, R32 src) {
    Assembler assembler;
    assembler.and_(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::AND_RM32_RM32);
    RM32 disdst = ins.op0<RM32>();
    RM32 dissrc = ins.op1<RM32>();
    verify(disdst.isReg);
    verify(dissrc.isReg);
    verify(disdst.reg == dst);
    verify(dissrc.reg == src);
}

void testAnd32(R32 dst, i32 imm) {
    Assembler assembler;
    assembler.and_(dst, imm);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::AND_RM32_IMM);
    RM32 disdst = ins.op0<RM32>();
    Imm dissrc = ins.op1<Imm>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc.as<i32>() == imm);
}

void testAnd32() {
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
    std::array<i32, 16> imms {{
        0,
        -1,
        1,
        -2,
        2,
        -4,
        4,
        255,
        -255,
        256,
        -256,
        (i32)0x12345678,
        (i32)0x87654321,
        (i32)0xABABABAB,
        (i32)0x7FFFFFFF,
        (i32)0xFFFFFFFF,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            testAnd32(dst, src);
        }
        for(auto imm : imms) {
            testAnd32(dst, imm);
        }
    }
}

void testAnd64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.and_(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::AND_RM64_RM64);
    RM64 disdst = ins.op0<RM64>();
    RM64 dissrc = ins.op1<RM64>();
    verify(disdst.isReg);
    verify(dissrc.isReg);
    verify(disdst.reg == dst);
    verify(dissrc.reg == src);
}

void testAnd64() {
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
            testAnd64(dst, src);
        }
    }
}

int main() {
    try {
        testAnd32();
        testAnd32();
        testAnd64();
    } catch(...) {
        return 1;
    }
}