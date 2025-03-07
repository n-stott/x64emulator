#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    Mmu mmu;
    Cpu cpu(mmu);

    std::array<X64Instruction, 2> instructions {{
        X64Instruction::make(0x0, Insn::PXOR_XMM_XMMM128, 1, XMM::XMM0, XMMM128{true, XMM::XMM0, {}}),
        X64Instruction::make(0x4, Insn::JMP_U32, 1, 0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(XMM::XMM0, Xmm{0x12345678, 0x98765432});

    u64 ticks { 0 };
    u64 basicBlockPtr { 0 };

    if(argc != 1) {
        cpu.exec(bb);
    } else {
        auto nativebb = Compiler::tryCompile(bb);
        if(!nativebb) {
            fmt::print(stderr, "failed to compile\n");
            return 1;
        }

        void* ptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(ptr == (void*)MAP_FAILED) return 1;
        ::memcpy(ptr, nativebb->nativecode.data(), nativebb->nativecode.size());
        cpu.exec((NativeExecPtr)ptr, &ticks, &basicBlockPtr);
    }

    fmt::print("XMM::XMM0={:x} {:x}\n", cpu.get(XMM::XMM0).hi, cpu.get(XMM::XMM0).lo);

    // if(cpu.get(R64::RAX) != 0x58) return 1;

    return 0;
}