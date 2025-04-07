#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    Mmu mmu;
    Cpu cpu(mmu);

    std::array<X64Instruction, 2> instructions {{
        X64Instruction::make(0x0, Insn::TEST_RM8_IMM, 1, RM8{true, R8::DH, {}} , Imm{0x30}),
        X64Instruction::make(0x1, Insn::JE, 1, (u64)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(R64::RDX, 0x3808);
    cpu.set(R64::RIP, 0x0);

    u64 ticks { 0 };
    u64 basicBlockPtr { 0 };

    if(argc != 1) {
        cpu.exec(bb);
    } else {
        auto trampoline = Compiler::tryCompileJitTrampoline();
        if(!trampoline) return 1;
        auto nativebb = Compiler::tryCompile(bb);
        if(!nativebb) return 1;

        void* trptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(trptr == (void*)MAP_FAILED) return 1;
        ::memcpy(trptr, trampoline->nativecode.data(), trampoline->nativecode.size());

        void* bbptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(bbptr == (void*)MAP_FAILED) return 1;
        ::memcpy(bbptr, nativebb->nativecode.data(), nativebb->nativecode.size());
        cpu.exec((NativeExecPtr)trptr, &ticks, &basicBlockPtr, (const u8*)bbptr);
    }

    fmt::print("{:#x}\n", cpu.get(R64::RIP));

    // if(cpu.get(R64::RAX) != 0x58) return 1;

    return 0;
}