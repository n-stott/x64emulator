#include "x64/cpu.h"
#include "x64/mmu.h"
#include "verify.h"
#include "fmt/core.h"
#include <optional>
#include <vector>

std::optional<u64> test_choice(u64 value, u64 value_if_zero, u64 value_if_nonzero) {
    using namespace x64;
    auto mmu = Mmu::tryCreateWithAddressSpace(1);
    if(!mmu) return {};

    struct alignas(16) Buffer {
        u64 zeroes { 0 };
        u64 ones { (u64)(-1) };
        u64 maskl { (u64)(~(u64)0xFF) };
        u64 masku { (u64)(~(u64)0xFF) };
        u64 scratch_val0 { 0 };
        u64 scratch_val1 { 0 };
        u64 scratch_val2 { 0 };
        u64 padding { 0 };
    };

    Buffer buffer;

    bool errorEncountered = false;
    std::optional<u64> choice;

    VerificationScope::run([&]() {
        auto dataPage = mmu->mmap(0, Mmu::PAGE_SIZE, BitFlags<PROT>{PROT::READ, PROT::WRITE}, BitFlags<MAP>{MAP::PRIVATE, MAP::ANONYMOUS});
        verify(!!dataPage);

        static_assert(sizeof(Buffer) >= 56);
        static_assert(alignof(Buffer) == 16);

        mmu->copyToMmu(Ptr8{dataPage.value()}, (const u8*)&buffer, sizeof(buffer));

        // RDI holds the buffer address
        // RSI holds value
        // RDX holds value_if_zero
        // RCX holds value_if_nonzero

        M128 zeroes_ones_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 0}};
        M128 mask_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 16}};
        M64 value_z_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 32}};
        M64 value_nz_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 40}};
        M128 value_znz_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 32}};
        M64 value_addr{Segment::UNK, Encoding64{R64::RDI, R64::ZERO, 1, 48}};

        std::vector<X64Instruction> instructions {
            X64Instruction::make<Insn::MOV_M64_R64>(0, 1, value_z_addr, R64::RDX),
            X64Instruction::make<Insn::MOV_M64_R64>(1, 1, value_nz_addr, R64::RCX),
            X64Instruction::make<Insn::MOV_M64_R64>(2, 1, value_addr, R64::RSI),
            X64Instruction::make<Insn::MOVDDUP_XMM_M64>(3, 1, XMM::XMM0, value_addr),
            X64Instruction::make<Insn::PAND_XMM_XMMM128>(4, 1, XMM::XMM0, XMMM128{false, XMM::XMM0, mask_addr}),
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(5, 1, XMM::XMM1, XMMM128{true, XMM::XMM1, {}}),
            X64Instruction::make<Insn::PCMPEQQ_XMM_XMMM128>(6, 1, XMM::XMM1, XMMM128{true, XMM::XMM0, {}}),
            X64Instruction::make<Insn::PXOR_XMM_XMMM128>(7, 1, XMM::XMM1, XMMM128{false, XMM::XMM0, zeroes_ones_addr}),
            X64Instruction::make<Insn::MOVAPS_XMMM128_XMMM128>(8, 1, XMMM128{true, XMM::XMM0, {}}, XMMM128{false, XMM::XMM0, value_znz_addr}),
            X64Instruction::make<Insn::PAND_XMM_XMMM128>(9, 1, XMM::XMM0, XMMM128{true, XMM::XMM1, {}}),
            X64Instruction::make<Insn::MOVHLPS_XMM_XMM>(10, 1, XMM::XMM1, XMM::XMM0),
            X64Instruction::make<Insn::POR_XMM_XMMM128>(11, 1, XMM::XMM0, XMMM128{true, XMM::XMM1, {}}),
            X64Instruction::make<Insn::MOVQ_RM64_XMM>(12, 1, RM64{false, R64::ZERO, value_addr}, XMM::XMM0),
            X64Instruction::make<Insn::MOV_R64_M64>(13, 1, R64::RAX, value_addr),
        };

        Cpu cpu(*mmu);
        cpu.set(R64::RDI, dataPage.value());
        cpu.set(R64::RSI, value);
        cpu.set(R64::RDX, value_if_zero);
        cpu.set(R64::RCX, value_if_nonzero);
        for(const auto& ins : instructions) {
            cpu.exec(ins);
        }
        choice = cpu.get(R64::RAX);
    }, [&]() {
        errorEncountered = true;
    });

    if(errorEncountered) return {};

    return choice;
}

void test(u64 value, u64 value_if_zero, u64 value_if_nonzero) {
    auto choice = test_choice(value, value_if_zero, value_if_nonzero);
    if(!choice) {
        fmt::println("test_choice({:#x}, {:#x}, {:#x}) failed", value, value_if_zero, value_if_nonzero);
    } else {
        fmt::println("test_choice({:#x}, {:#x}, {:#x}) = {:#x}", value, value_if_zero, value_if_nonzero, choice.value());
    }
}

int main() {
    test(0x0, 0x1234, 0xabcd);
    test(0xFF, 0x1234, 0xabcd);
    test(0x100, 0x1234, 0xabcd);
    test(0x1023, 0x1234, 0xabcd);
}