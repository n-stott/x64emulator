#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testMovsx6432(R64 dst, R32 src) {
    Assembler assembler;
    assembler.movsx(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOVSX_R64_RM32);
    auto disdst = ins.op0<R64>();
    auto dissrc = ins.op1<RM32>();
    verify(disdst == dst);
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testMovsx6432() {
    std::array<R32, 16> srcregs {{
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
    std::array<R64, 16> dstregs {{
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
    for(auto dst : dstregs) {
        for(auto src : srcregs) {
            testMovsx6432(dst, src);
        }
    }
}

int main() {
    try {
        testMovsx6432();
    } catch(...) {
        return 1;
    }
}