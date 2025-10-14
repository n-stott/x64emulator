#include "x64/compiler/assembler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "verify.h"

using namespace x64;

void testMov16(R16 dst, R16 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R16_R16);
    R16 disdst = ins.op0<R16>();
    R16 dissrc = ins.op1<R16>();
    verify(disdst == dst);
    verify(dissrc == src);
}

void testMov16() {
    std::array<R16, 16> regs {{
        R16::AX,
        R16::CX,
        R16::DX,
        R16::BX,
        R16::SP,
        R16::BP,
        R16::SI,
        R16::DI,
        R16::R8W,
        R16::R9W,
        R16::R10W,
        R16::R11W,
        R16::R12W,
        R16::R13W,
        R16::R14W,
        R16::R15W,
    }};
    for(auto dst : regs) {
        for(auto src : regs) {
            testMov16(dst, src);
        }
    }
}

void testMov32(R32 dst, R32 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R32_R32);
    R32 disdst = ins.op0<R32>();
    R32 dissrc = ins.op1<R32>();
    verify(disdst == dst);
    verify(dissrc == src);
}

void testMov32() {
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
            testMov32(dst, src);
        }
    }
}

void testMov64(R64 dst, R64 src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R64_R64);
    verify(ins.op0<R64>() == dst);
    verify(ins.op1<R64>() == src);
}

void testMov64(R64 dst, const M64& src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_R64_M64);
    verify(ins.op0<R64>() == dst);
    M64 dissrc = ins.op1<M64>();
    verify(dissrc.segment == src.segment);
    verify(dissrc.encoding.base == src.encoding.base);
    verify(dissrc.encoding.index == src.encoding.index);
    verify(dissrc.encoding.scale == src.encoding.scale);
    verify(dissrc.encoding.displacement == src.encoding.displacement);
}

void testMov64() {
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
            testMov64(dst, src);
            M64 s { Segment::UNK, Encoding64{src, R64::ZERO, 1, 0} };
            testMov64(dst, s);
            for(auto index : regs) {
                if(index == R64::RSP) continue;
                if(index == R64::R12) continue;
                M64 s1 { Segment::UNK, Encoding64{src, index, 1, 0} };
                M64 s2 { Segment::UNK, Encoding64{src, index, 2, 0} };
                M64 s4 { Segment::UNK, Encoding64{src, index, 4, 0} };
                M64 s8 { Segment::UNK, Encoding64{src, index, 8, 0} };
                testMov64(dst, s1);
                testMov64(dst, s2);
                testMov64(dst, s4);
                testMov64(dst, s8);

                M64 s17 { Segment::UNK, Encoding64{src, index, 1, 7} };
                M64 s27 { Segment::UNK, Encoding64{src, index, 2, 7} };
                M64 s47 { Segment::UNK, Encoding64{src, index, 4, 7} };
                M64 s87 { Segment::UNK, Encoding64{src, index, 8, 7} };
                testMov64(dst, s17);
                testMov64(dst, s27);
                testMov64(dst, s47);
                testMov64(dst, s87);

                M64 s11024 { Segment::UNK, Encoding64{src, index, 1, 1024} };
                M64 s21024 { Segment::UNK, Encoding64{src, index, 2, 1024} };
                M64 s41024 { Segment::UNK, Encoding64{src, index, 4, 1024} };
                M64 s81024 { Segment::UNK, Encoding64{src, index, 8, 1024} };
                testMov64(dst, s11024);
                testMov64(dst, s21024);
                testMov64(dst, s41024);
                testMov64(dst, s81024);
            }
        }
    }
}

void testMov128(XMM dst, XMM src) {
    Assembler assembler;
    assembler.mov(dst, src);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_XMM_XMM);
    verify(ins.op0<XMM>() == dst);
    verify(ins.op1<XMM>() == src);
}

void testMov128() {
    std::array<XMM, 16> regs {{
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
    for(auto dst : regs) {
        for(auto src : regs) {
            testMov128(dst, src);
        }
    }
}

void testMovM64Imm32(const M64& dst, u32 imm) {
    Assembler assembler;
    assembler.mov(dst, imm);
    std::vector<u8> code = assembler.code();
    CapstoneWrapper disassembler;
    auto disassembly = disassembler.disassembleRange(code.data(), code.size(), 0x0);
    verify(disassembly.instructions.size() == 1);
    const auto& ins = disassembly.instructions[0];
    verify(ins.insn() == Insn::MOV_M64_IMM);
    M64 disdst = ins.op0<M64>();
    Imm disimm = ins.op1<Imm>();
    verify(disdst == dst);
    verify(disimm.as<u32>() == imm);
}

void testMovM64Imm32() {
    std::array<R64, 14> bases {{
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
        R64::R12,
        R64::R13,
        R64::R14,
        R64::R15,
    }};
    std::array<R64, 14> indexes {{
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
        R64::R12,
        R64::R13,
        R64::R14,
        R64::R15,
    }};
    std::array<u8, 3> scales {{
        1, 2, 4
    }};
    std::array<i32, 8> diss {{
        0, 1,  2,  4,
        8, 16, -2, -4
    }};
    std::array<u32, 8> imms {{
        +0, +1, +2, +3,
        (u32)-1, (u32)-2, (u32)-3, (u32)-4
    }};
    for(auto base : bases) {
        for(auto index : indexes) {
            for(auto scale : scales) {
                for(auto dis : diss) {
                    for(auto imm : imms) {
                        M64 mem { Segment::UNK, Encoding64{base, index, scale, dis} };
                        testMovM64Imm32(mem, imm);
                    }
                }
            }
        }
    }
}

int main() {
    try {
        testMov32();
        testMov64();
        testMov128();
        testMovM64Imm32();
    } catch(...) {
        return 1;
    }
}