#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testCmov32(Cond cond, R32 dst, R32 src) {
    Assembler assembler;
    assembler.cmov(cond, dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::CMOV_R32_RM32);
    Cond discond = ins.op0<Cond>();
    R32 disdst = ins.op1<R32>();
    RM32 dissrc = ins.op2<RM32>();
    verify(discond == cond);
    verify(disdst == dst);
    verify(dissrc.isReg);
    verify(dissrc.reg == src);
}

void testCmov32() {
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
    std::array<Cond, 14> conds {{
        Cond::B,
        Cond::AE,
        Cond::E,
        Cond::NE,
        Cond::BE,
        Cond::A,
        Cond::S,
        Cond::NS,
        Cond::P,
        Cond::NP,
        Cond::L,
        Cond::GE,
        Cond::LE,
        Cond::G,
    }};
    for(auto cond : conds) {
        for(auto dst : regs) {
            for(auto src : regs) {
                testCmov32(cond, dst, src);
            }
        }
    }
}

int main() {
    try {
        testCmov32();
    } catch(...) {
        return 1;
    }
}