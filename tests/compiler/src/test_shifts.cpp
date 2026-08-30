#include "arch/x64/compiler/assembler.h"
#include "arch/x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testShl32() {
    Assembler assembler;
    assembler.shl(R32::EAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHL_RM32_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testShl64() {
    Assembler assembler;
    assembler.shl(R64::RAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHL_RM64_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}


void testShr8() {
    Assembler assembler;
    assembler.shr(R8::AL, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHR_RM8_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testShr16() {
    Assembler assembler;
    assembler.shr(R16::AX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHR_RM16_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testShr32() {
    Assembler assembler;
    assembler.shr(R32::EAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHR_RM32_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testShr64() {
    Assembler assembler;
    assembler.shr(R64::RAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SHR_RM64_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}


void testSar16() {
    Assembler assembler;
    assembler.sar(R16::AX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SAR_RM16_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testSar32() {
    Assembler assembler;
    assembler.sar(R32::EAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SAR_RM32_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testSar64() {
    Assembler assembler;
    assembler.sar(R64::RAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::SAR_RM64_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}


void testRol16() {
    Assembler assembler;
    assembler.rol(R16::AX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::ROL_RM16_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testRol32() {
    Assembler assembler;
    assembler.rol(R32::EAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::ROL_RM32_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testRol64() {
    Assembler assembler;
    assembler.rol(R64::RAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::ROL_RM64_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}


void testRor32() {
    Assembler assembler;
    assembler.ror(R32::EAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::ROR_RM32_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

void testRor64() {
    Assembler assembler;
    assembler.ror(R64::RAX, R8::DL);

    ZydisWrapper dis;
    auto res = dis.disassembleRange(assembler.code().data(), assembler.code().size(), 0);
    verify(res.instructions.size() == 4);
    verify(res.instructions[0].insn() == Insn::PUSH_RM64);
    verify(res.instructions[1].insn() == Insn::MOV_R8_R8);
    verify(res.instructions[2].insn() == Insn::ROR_RM64_R8);
    verify(res.instructions[3].insn() == Insn::POP_R64);
}

int main() {
    try {
        testShl32();
        testShl64();

        testShr8();
        testShr16();
        testShr32();
        testShr64();

        testSar16();
        testSar32();
        testSar64();

        testRol16();
        testRol32();
        testRol64();

        testRor32();
        testRor64();
    } catch(...) {
        return 1;
    }
    return 0;
}