#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "x64/tostring.h"
#include "verify.h"

using namespace x64;

void testLea64(R64 dst, const M64& src) {
    Assembler assembler;
    assembler.lea(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1, fmt::format("Disassembly of \"lea {}, {}\" failed", utils::toString(dst), utils::toString(src)));
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::LEA_R64_ENCODING64, "Not an LEA");
    R64 disdst = ins.op0<R64>();
    Encoding64 dissrc = ins.op1<Encoding64>();
    verify(disdst == dst, fmt::format("Expected dst={}, got {}", utils::toString(dst), utils::toString(disdst)));
    verify(dissrc.base == src.encoding.base, fmt::format("Expected src={}, got {}", utils::toString(src.encoding), utils::toString(dissrc)));
    verify(dissrc.index == src.encoding.index, fmt::format("Expected src={}, got {}", utils::toString(src.encoding), utils::toString(dissrc)));
    if(src.encoding.index != R64::ZERO)
        verify(dissrc.scale == src.encoding.scale, fmt::format("Expected src={}, got {}", utils::toString(src.encoding), utils::toString(dissrc)));
    verify(dissrc.displacement == src.encoding.displacement, fmt::format("Expected src={}, got {}", utils::toString(src.encoding), utils::toString(dissrc)));
}

void testLea64() {
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
                for(auto index : regs) {
                    if(index == R64::RSP) continue;
                    for(u8 scale : { (u8)1, (u8)2, (u8)4, (u8)8 }) {
                        M64 src {
                            Segment::CS,
                            Encoding64 {
                                base,
                                index,
                                scale,
                                imm
                            },
                        };
                        testLea64(dst, src);
                    }
                }
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