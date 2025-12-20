#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "x64/compiler/jit.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    auto mmu = Mmu::tryCreateWithAddressSpace(1);
    if(!mmu) return 1;
    Cpu cpu(*mmu);

    std::array<X64Instruction, 2> instructions {{
        // shufps xmm0, xmm0, 0xff
        X64Instruction::make(0x0, Insn::SHUFPS_XMM_XMMM128_IMM, 1, XMM::XMM0, XMMM128{true, XMM::XMM0, {}}, Imm{0xff}),
        // jmp 0x0
        X64Instruction::make(0xc, Insn::JMP_U32, 1, (u32)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(XMM::XMM0, u128{0x1234567887654321, 0x8765432112345678});

    u64 ticks { 0 };
    std::array<u64, 0x100> basicBlockData;
    std::fill(basicBlockData.begin(), basicBlockData.end(), 0);
    void* basicBlockPtr = &basicBlockData;
    
    std::array<u64, 0x100> jitBasicBlockData;
    std::fill(jitBasicBlockData.begin(), jitBasicBlockData.end(), 0);

    if(argc != 1) {
        cpu.exec(bb);
    } else {
        Compiler compiler;
        auto nativebb = compiler.tryCompile(bb);
        if(!nativebb) return 1;

        void* bbptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(bbptr == (void*)MAP_FAILED) return 1;
        ::memcpy(bbptr, nativebb->nativecode.data(), nativebb->nativecode.size());

        auto jit = Jit::tryCreate();
        jit->exec(&cpu, mmu.get(), (NativeExecPtr)bbptr, &ticks, &basicBlockPtr, &jitBasicBlockData);
    }

    fmt::print("XMM0={:x} {:x}\n", cpu.get(XMM::XMM0).hi, cpu.get(XMM::XMM0).lo);

    return 0;
}