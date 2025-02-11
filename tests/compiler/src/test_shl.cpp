#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testShl32(R32 dst) {
    Assembler assembler;
    assembler.shl_cl(dst);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::SHL_RM32_R8);
    RM32 disdst = ins.op0<RM32>();
    R8 dissrc = ins.op1<R8>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc == R8::CL);
}

void testShl32(R32 dst, u8 imm) {
    Assembler assembler;
    assembler.shl(dst, imm);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::SHL_RM32_IMM);
    RM32 disdst = ins.op0<RM32>();
    Imm disimm = ins.op1<Imm>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(disimm.as<u8>() == imm);
}

void testShl32() {
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
    std::array<u8, 10> imms {{
        0,
        1,
        2,
        4,
        8,
        16,
        17,
        18,
        20,
        31,
    }};
    for(auto dst : regs) {
        testShl32(dst);
        for(auto imm : imms) {
            testShl32(dst, imm);
        }
    }
}

void testShl64(R64 dst) {
    Assembler assembler;
    assembler.shl_cl(dst);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::SHL_RM64_R8);
    RM64 disdst = ins.op0<RM64>();
    R8 dissrc = ins.op1<R8>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc == R8::CL);
}

void testShl64(R64 dst, u8 imm) {
    Assembler assembler;
    assembler.shl(dst, imm);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::SHL_RM64_IMM);
    RM64 disdst = ins.op0<RM64>();
    Imm dissrc = ins.op1<Imm>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc.as<u8>() == imm);
}

void testShl64() {
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
    std::array<u8, 16> imms {{
        0,
        1,
        2,
        4,
        8,
        16,
        17,
        18,
        20,
        31,
        40,
        50,
        61,
        62,
        63,
    }};
    for(auto dst : regs) {
        testShl64(dst);
        for(auto imm : imms) {
            testShl64(dst, imm);
        }
    }
}

int main() {
    try {
        testShl32();
        testShl64();
    } catch(...) {
        return 1;
    }
}