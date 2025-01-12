#include "x64/cpu.h"
#include "x64/mmu.h"
#include "verify.h"
#include "fmt/core.h"
#include <vector>

std::vector<char> string {{
    'h', 'e', 'l', 'l', 'o', '!', '\0'
}};

int main() {
    using namespace x64;
    Mmu mmu;

    u64 length = 0;

    bool errorEncountered = false;

    VerificationScope::run([&]() {
        u64 dataPage = mmu.mmap(0, Mmu::PAGE_SIZE, BitFlags<PROT>{PROT::READ, PROT::WRITE}, BitFlags<MAP>{MAP::PRIVATE, MAP::ANONYMOUS});
        mmu.copyToMmu(Ptr8{dataPage}, (const u8*)string.data(), string.size());

        std::vector<X64Instruction> instructions {
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(1, 1, XMM::XMM0, XMMM128{true, XMM::XMM0, {}}),
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(2, 1, XMM::XMM1, XMMM128{true, XMM::XMM1, {}}),
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(3, 1, XMM::XMM2, XMMM128{true, XMM::XMM2, {}}),
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(4, 1, XMM::XMM3, XMMM128{true, XMM::XMM3, {}}),
            X64Instruction::make<Insn::MOV_R64_R64>(5, 1, R64::RAX, R64::RDI),
            X64Instruction::make<Insn::MOV_R64_R64>(6, 1, R64::RCX, R64::RDI),
            X64Instruction::make<Insn::AND_RM64_IMM>(7, 1, RM64{true, R64::RCX, {}}, Imm{0xfff}),
            X64Instruction::make<Insn::MOV_UNALIGNED_XMM_M128>(8, 1, XMM::XMM4, M128{Segment::UNK, Encoding64{R64::RAX, R64::ZERO, 1, 0 }}),
            X64Instruction::make<Insn::PCMPEQB_XMM_XMMM128>(9, 1, XMM::XMM4, XMMM128{true, XMM::XMM0, {}}),
            X64Instruction::make<Insn::PMOVMSKB_R32_XMM>(10, 1, R32::EDX, XMM::XMM4),
            X64Instruction::make<Insn::TEST_RM32_R32>(11, 1, RM32{true, R32::EDX, {}}, R32::EDX),
            X64Instruction::make<Insn::BSF_R32_R32>(12, 1, R32::EAX, R32::EDX)
        };

        Cpu cpu(mmu);
        cpu.set(R64::RDI, dataPage);
        for(const auto& ins : instructions) {
            cpu.exec(ins);
        }
        length = cpu.get(R64::RAX);
    }, [&]() {
        errorEncountered = true;
    });

    if(errorEncountered) return 1;

    fmt::print("Test string       : \"{}\"\n", string.data());
    fmt::print("VM computed length: {}\n", length);
    fmt::print("Actual length     : {}\n", strlen(string.data()));

    if(length != 6) return 1;
    return 0;
}