#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testImul32(R32 dst, R32 src) {
    Assembler assembler;
    assembler.imul(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
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

// void testImul32(R32 dst, i32 imm) {
//     Assembler assembler;
//     assembler.imul(dst, imm);
//     std::vector<u8> code = assembler.code();
//     auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
//     verify(disassembly.instructions.size() == 1);
//     const auto& ins = disassembly.instructions[0];
//     verify(ins.insn() == Insn::IMUL_RM32_IMM);
//     RM32 disdst = ins.op0<RM32>();
//     Imm disimm = ins.op1<Imm>();
//     verify(disdst.isReg);
//     verify(disdst.reg == dst);
//     verify(disimm.as<i32>() == imm);
// }

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

// void testImul64(R64 dst, R64 src) {
//     Assembler assembler;
//     assembler.imul(dst, src);
//     std::vector<u8> code = assembler.code();
//     auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
//     verify(disassembly.instructions.size() == 1);
//     const auto& ins = disassembly.instructions[0];
//     verify(ins.insn() == Insn::IMUL_RM64_RM64);
//     RM64 disdst = ins.op0<RM64>();
//     RM64 dissrc = ins.op1<RM64>();
//     verify(disdst.isReg);
//     verify(dissrc.isReg);
//     verify(disdst.reg == dst);
//     verify(dissrc.reg == src);
// }

// void testImul64(R64 dst, i32 imm) {
//     Assembler assembler;
//     assembler.imul(dst, imm);
//     std::vector<u8> code = assembler.code();
//     auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
//     verify(disassembly.instructions.size() == 1);
//     const auto& ins = disassembly.instructions[0];
//     verify(ins.insn() == Insn::IMUL_RM64_IMM);
//     RM64 disdst = ins.op0<RM64>();
//     Imm dissrc = ins.op1<Imm>();
//     verify(disdst.isReg);
//     verify(disdst.reg == dst);
//     verify(dissrc.as<i32>() == imm);
// }

// void testImul64() {
//     std::array<R64, 16> regs {{
//         R64::RAX,
//         R64::RCX,
//         R64::RDX,
//         R64::RBX,
//         R64::RSP,
//         R64::RBP,
//         R64::RSI,
//         R64::RDI,
//         R64::R8,
//         R64::R9,
//         R64::R10,
//         R64::R11,
//         R64::R12,
//         R64::R13,
//         R64::R14,
//         R64::R15,
//     }};
//     std::array<i32, 16> imms {{
//         0,
//         -1,
//         1,
//         -2,
//         2,
//         -4,
//         4,
//         255,
//         -255,
//         256,
//         -256,
//         (i32)0x12345678,
//         (i32)0x87654321,
//         (i32)0xABABABAB,
//         (i32)0x7FFFFFFF,
//         (i32)0xFFFFFFFF,
//     }};
//     for(auto dst : regs) {
//         for(auto src : regs) {
//             testImul64(dst, src);
//         }
//         for(auto imm : imms) {
//             testImul64(dst, imm);
//         }
//     }
// }

int main() {
    try {
        testImul32();
        // testImul64();
    } catch(...) {
        return 1;
    }
}