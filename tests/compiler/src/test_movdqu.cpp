#include "x64/compiler/assembler.h"
#include "x64/disassembler/zydiswrapper.h"
#include "verify.h"

using namespace x64;

void testMovdqu64(XMM dst, const M128& src) {
    Assembler assembler;
    assembler.movu(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_UNALIGNED_XMM_M128);
    XMM disdst = ins.op0<XMM>();
    M128 dissrc = ins.op1<M128>();
    verify(disdst == dst);
    verify(dissrc.encoding.base == src.encoding.base);
    verify(dissrc.encoding.index == src.encoding.index);
    verify(dissrc.encoding.displacement == src.encoding.displacement);
}

void testMovdqu64(const M128& dst, XMM src) {
    Assembler assembler;
    assembler.movu(dst, src);
    std::vector<u8> code = assembler.code();
    ZydisWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_UNALIGNED_M128_XMM);
    M128 disdst = ins.op0<M128>();
    XMM dissrc = ins.op1<XMM>();
    verify(disdst.encoding.base == dst.encoding.base);
    verify(disdst.encoding.index == dst.encoding.index);
    verify(disdst.encoding.displacement == dst.encoding.displacement);
    verify(dissrc == src);
}

void testMovdqu64() {
    std::array<R64, 14> regs {{
        R64::RAX,
        R64::RCX,
        R64::RDX,
        R64::RBX,
        R64::RSP,
        R64::RSI,
        R64::RDI,
        R64::R8,
        R64::R9,
        R64::R10,
        R64::R11,
        R64::R12,
        R64::R14,
        R64::R15,
    }};
    std::array<XMM, 16> xmms {{
        XMM::XMM0,
        XMM::XMM1,
        XMM::XMM2,
        XMM::XMM3,
        XMM::XMM4,
        XMM::XMM5,
        XMM::XMM6,
        XMM::XMM7,
        XMM::XMM8,
        XMM::XMM9,
        XMM::XMM10,
        XMM::XMM11,
        XMM::XMM12,
        XMM::XMM13,
        XMM::XMM14,
        XMM::XMM15,
    }};
    for(auto r64 : regs) {
        for(i32 disp : { -256, -16, 0, 10, 256 }) {
            auto m = M128 { Segment::DS, Encoding64 { r64, R64::ZERO, 1, disp } };
            for(auto xmm : xmms) {
                testMovdqu64(m, xmm);
                testMovdqu64(xmm, m);
            }
        }
    }
}

int main() {
    try {
        testMovdqu64();
    } catch(...) {
        return 1;
    }
}