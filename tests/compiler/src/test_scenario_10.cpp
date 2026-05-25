#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "x64/compiler/jit.h"
#include <sys/mman.h>
#include <cstdio>


int main(int, char**) {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(1);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);
    Cpu cpu(mmu);

    // mov  dl,r8b
    // mov  dh,bl

    std::array<X64Instruction, 3> instructions {{
        X64Instruction::make(0x4, Insn::MOV_R8_R8, 1, R8::DL, R8::R8B),
        X64Instruction::make(0x5, Insn::MOV_R8_R8, 1, R8::DH, R8::BL),
        X64Instruction::make(0x6, Insn::JNE, 1, (u64)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    CompilerOptions compileroptions = CompilerOptions::full();
    Compiler compiler(compileroptions);
    auto nativebb = compiler.tryCompile(bb);
    if(!nativebb) return 1;

    void* bbptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
    if(bbptr == (void*)MAP_FAILED) return 1;
    ::memcpy(bbptr, nativebb->nativecode.data(), nativebb->nativecode.size());

    u64 ticks { 0 };
    std::array<u64, 0x100> basicBlockData;
    std::fill(basicBlockData.begin(), basicBlockData.end(), 0);
    void* basicBlockPtr = &basicBlockData;
    
    std::array<u64, 0x100> jitBasicBlockData;
    std::fill(jitBasicBlockData.begin(), jitBasicBlockData.end(), 0);

    Jit::Options options = Jit::Options::full();
    // options.directGpr = false;
    auto jit = Jit::tryCreate(options);

    auto compare = [&](u8 bl, u64 rdx, u8 r8b) {
        cpu.set(R8::BL, bl);
        cpu.set(R64::RDX, rdx);
        cpu.set(R8::R8B, r8b);

        cpu.exec(bb);

        u64 cpurdx = cpu.get(R64::RDX);


        cpu.set(R8::BL, bl);
        cpu.set(R64::RDX, rdx);
        cpu.set(R8::R8B, r8b);

        jit->exec(&cpu, &mmu, (NativeExecPtr)bbptr, &ticks, &basicBlockPtr, &jitBasicBlockData);


        u64 jitrdx = cpu.get(R64::RDX);

        fmt::print("\nRDX={:x} {:x}\n", cpurdx, jitrdx);

        if(cpurdx != jitrdx) throw 1;
    };

    srand(0);
    auto val = []() -> int { return rand(); };
    compare((u8)(val()), (u64)(val()), (u8)(val()));


    return 0;
}