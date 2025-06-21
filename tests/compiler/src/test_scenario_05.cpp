#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "x64/compiler/jit.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    auto mmu = Mmu::tryCreate(1);
    if(!mmu) return 1;
    Cpu cpu(*mmu);

    std::array<X64Instruction, 2> instructions {{
        X64Instruction::make(0x0, Insn::PXOR_XMM_XMMM128, 1, XMM::XMM0, XMMM128{true, XMM::XMM0, {}}),
        X64Instruction::make(0x4, Insn::JMP_U32, 1, (u32)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(XMM::XMM0, Xmm{0x12345678, 0x98765432});

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

    fmt::print("XMM::XMM0={:x} {:x}\n", cpu.get(XMM::XMM0).hi, cpu.get(XMM::XMM0).lo);

    // if(cpu.get(R64::RAX) != 0x58) return 1;

    return 0;
}