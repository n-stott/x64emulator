#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    Mmu mmu;
    Cpu cpu(mmu);

    std::array<X64Instruction, 2> instructions {{
        X64Instruction::make(0x0, Insn::MOVZX_R32_RM8, 1, R32::EAX, RM8{true, R8::CL, {}}),
        X64Instruction::make(0x1, Insn::JMP_U32, 1, Imm{0x0}),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(R64::RAX, 0x20);
    cpu.set(R64::RCX, 0x1055cd58);
    cpu.set(R64::RIP, 0x0);

    if(argc != 1) {
        cpu.exec(bb);
    } else {
        auto nativebb = Compiler::tryCompile(bb);
        if(!nativebb) return 1;

        void* ptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(ptr == (void*)MAP_FAILED) return 1;
        ::memcpy(ptr, nativebb->nativecode.data(), nativebb->nativecode.size());
        cpu.exec((NativeExecPtr)ptr);
    }

    if(cpu.get(R64::RAX) != 0x58) return 1;

    return 0;
}