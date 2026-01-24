#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "x64/compiler/jit.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(1);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);
    Cpu cpu(mmu);

    std::array<X64Instruction, 11> instructions {{
        X64Instruction::make(0x0, Insn::NOP, 1),
        X64Instruction::make(0x1, Insn::POP_R64, 1, R64::RBX),
        X64Instruction::make(0x2, Insn::POP_R64, 1, R64::R14),
        X64Instruction::make(0x3, Insn::POP_R64, 1, R64::R13),
        X64Instruction::make(0x4, Insn::POP_R64, 1, R64::R12),
        X64Instruction::make(0x5, Insn::POP_R64, 1, R64::R11),
        X64Instruction::make(0x6, Insn::POP_R64, 1, R64::R10),
        X64Instruction::make(0x7, Insn::MOV_R64_R64, 1, R64::RSP, R64::RBP),
        X64Instruction::make(0x8, Insn::POP_R64, 1, R64::RSP),
        X64Instruction::make(0x9, Insn::POP_R64, 1, R64::RBP),
        X64Instruction::make(0xa, Insn::RET, 1),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    auto stack1base = mmu.mmap(0, 0x1000, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
    auto stack2base = mmu.mmap(0, 0x1000, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});

    if(!stack1base) return 1;
    if(!stack2base) return 1;
    u64 stack1Top = stack1base.value() + 0x1000;
    u64 stack2Top = stack2base.value() + 0x1000;

    cpu.set(R64::RBX, 0xb);
    cpu.set(R64::R10, 0x10);
    cpu.set(R64::R11, 0x11);
    cpu.set(R64::R12, 0x12);
    cpu.set(R64::R13, 0x13);
    cpu.set(R64::R14, 0x14);
    cpu.set(R64::RIP, 0xabcd);

    cpu.set(R64::RBP, stack1Top);
    cpu.set(R64::RSP, stack2Top);

    cpu.push64(cpu.get(R64::RIP));
    cpu.push64(cpu.get(R64::RBP));
    cpu.push64(cpu.get(R64::RSP));
    cpu.set(R64::RBP, cpu.get(R64::RSP));
    cpu.push64(cpu.get(R64::R10));
    cpu.push64(cpu.get(R64::R11));
    cpu.push64(cpu.get(R64::R12));
    cpu.push64(cpu.get(R64::R13));
    cpu.push64(cpu.get(R64::R14));
    cpu.push64(cpu.get(R64::RBX));

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
        jit->exec(&cpu, &mmu, (NativeExecPtr)bbptr, &ticks, &basicBlockPtr, &jitBasicBlockData);
    }

    fmt::print("R10={:#x}\n", cpu.get(R64::R10));
    fmt::print("R11={:#x}\n", cpu.get(R64::R11));
    fmt::print("R12={:#x}\n", cpu.get(R64::R12));
    fmt::print("R13={:#x}\n", cpu.get(R64::R13));
    fmt::print("R14={:#x}\n", cpu.get(R64::R14));
    fmt::print("RBX={:#x}\n", cpu.get(R64::RBX));
    fmt::print("RSP={:#x}\n", cpu.get(R64::RSP));
    fmt::print("RBP={:#x}\n", cpu.get(R64::RBP));
    fmt::print("RIP={:#x}\n", cpu.get(R64::RIP));

    // if(cpu.get(R64::RAX) != 0x58) return 1;

    return 0;
}