#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testAnd32(R32 dst, i32 imm) {
    Assembler assembler;
    assembler.and_(dst, imm);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::AND_RM32_IMM);
    RM32 disdst = ins.op0<RM32>();
    Imm dissrc = ins.op1<Imm>();
    verify(disdst.isReg);
    verify(disdst.reg == dst);
    verify(dissrc.as<i32>() == imm);
}

void testTest64() {
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
        for(auto imm : imms) {
            testAnd32(dst, imm);
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