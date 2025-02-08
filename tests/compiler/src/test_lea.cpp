#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testLea64(R64 dst, const M64& src) {
    Assembler assembler;
    assembler.lea(dst, src);
    std::vector<u8> code = assembler.code();
    auto disassembly = CapstoneWrapper::disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::LEA_R64_ENCODING64);
    R64 disdst = ins.op0<R64>();
    Encoding64 dissrc = ins.op1<Encoding64>();
    verify(disdst == dst);
    verify(dissrc.base == src.encoding.base);
    verify(dissrc.index == src.encoding.index);
    verify(dissrc.scale == src.encoding.scale);
    verify(dissrc.displacement == src.encoding.displacement);
}

void testLea64() {
    std::array<R64, 12> regs {{
        R64::RAX,
        R64::RCX,
        R64::RDX,
        R64::RBX,
        R64::RSI,
        R64::RDI,
        R64::R8,
        R64::R9,
        R64::R10,
        R64::R11,
        R64::R14,
        R64::R15,
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
        for(auto base : regs) {
            for(auto imm : imms) {
                M64 src {
                    Segment::CS,
                    Encoding64 {
                        base,
                        R64::ZERO,
                        1,
                        imm
                    },
                };
                testLea64(dst, src);
            }
        }
    }
}

int main() {
    try {
        testLea64();
    } catch(...) {
        return 1;
    }
}