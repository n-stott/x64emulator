#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    auto mmu = Mmu::tryCreate(1);
    if(!mmu) return 1;
    Cpu cpu(*mmu);

    std::array<X64Instruction, 12> instructions {{
        // cvttsd2si rax,xmm1
        X64Instruction::make(0x0, Insn::CVTTSD2SI_R64_XMM, 1, R64::RAX, XMM::XMM1),
        // pxor      xmm4,xmm4
        X64Instruction::make(0x1, Insn::PXOR_XMM_XMMM128, 1, XMM::XMM4, XMMM128{true, XMM::XMM4, {}}),
        // andnpd    xmm0,xmm1
        X64Instruction::make(0x2, Insn::ANDNPD_XMM_XMMM128, 1, XMM::XMM0, XMMM128{true, XMM::XMM1, {}}),
        // cvtsi2sd  xmm4,rax
        X64Instruction::make(0x3, Insn::CVTSI2SD_XMM_RM64, 1, XMM::XMM4, RM64{true, R64::RAX, {}}),
        // mov       xmm2,xmm4
        X64Instruction::make(0x4, Insn::MOV_XMM_XMM, 1, XMM::XMM2, XMM::XMM4),
        // cmpsd     xmm2,xmm1
        X64Instruction::make(0x5, Insn::CMPSD_XMM_XMM, 1, XMM::XMM2, XMM::XMM1, FCond::NLE),
        // andpd     xmm2,xmm3
        X64Instruction::make(0x6, Insn::ANDPD_XMM_XMMM128, 1, XMM::XMM2, XMMM128{true, XMM::XMM3, {}}),
        // subsd     xmm4,xmm2
        X64Instruction::make(0x7, Insn::SUBSD_XMM_XMM, 1, XMM::XMM4, XMM::XMM2),
        // mov       xmm1,xmm4
        X64Instruction::make(0x8, Insn::MOV_XMM_XMM, 1, XMM::XMM1, XMM::XMM4),
        // orpd      xmm1,xmm0
        X64Instruction::make(0x9, Insn::ORPD_XMM_XMMM128, 1, XMM::XMM1, XMMM128{true, XMM::XMM0, {}}),
        // cvttsd2si eax,xmm1
        X64Instruction::make(0xa, Insn::CVTTSD2SI_R32_XMM, 1, R32::EAX, XMM::XMM1),
        // mov       DWORD PTR [rcx],eax
        // mov       eax,0x1
        // jmp 0
        X64Instruction::make(0xb, Insn::JMP_U32, 1, (u32)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());


    cpu.set(XMM::XMM0, u128{0x7fffffffffffffff, 0});
    cpu.set(XMM::XMM1, u128{0xc074480000000000, 0});
    cpu.set(XMM::XMM2, u128{0x4330000000000000, 0});
    cpu.set(XMM::XMM3, u128{0x3ff0000000000000, 0});
    cpu.set(XMM::XMM4, u128{0x4074480000000000, 0});


    fmt::print("XMM0={:x} {:x}\n", cpu.get(XMM::XMM0).hi, cpu.get(XMM::XMM0).lo);
    fmt::print("XMM1={:x} {:x}\n", cpu.get(XMM::XMM1).hi, cpu.get(XMM::XMM1).lo);
    fmt::print("XMM2={:x} {:x}\n", cpu.get(XMM::XMM2).hi, cpu.get(XMM::XMM2).lo);
    fmt::print("XMM3={:x} {:x}\n", cpu.get(XMM::XMM3).hi, cpu.get(XMM::XMM3).lo);
    fmt::print("XMM4={:x} {:x}\n", cpu.get(XMM::XMM4).hi, cpu.get(XMM::XMM4).lo);

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
        auto trampoline = compiler.tryCompileJitTrampoline();
        if(!trampoline) return 1;
        auto nativebb = compiler.tryCompile(bb);
        if(!nativebb) return 1;

        void* trptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(trptr == (void*)MAP_FAILED) return 1;
        ::memcpy(trptr, trampoline->nativecode.data(), trampoline->nativecode.size());

        void* bbptr = ::mmap(nullptr, 0x1000, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(bbptr == (void*)MAP_FAILED) return 1;
        ::memcpy(bbptr, nativebb->nativecode.data(), nativebb->nativecode.size());
        cpu.exec((NativeExecPtr)trptr, (NativeExecPtr)bbptr, &ticks, &basicBlockPtr, &jitBasicBlockData);
    }

    fmt::print("XMM0={:x} {:x}\n", cpu.get(XMM::XMM0).hi, cpu.get(XMM::XMM0).lo);
    fmt::print("XMM1={:x} {:x}\n", cpu.get(XMM::XMM1).hi, cpu.get(XMM::XMM1).lo);
    fmt::print("XMM2={:x} {:x}\n", cpu.get(XMM::XMM2).hi, cpu.get(XMM::XMM2).lo);
    fmt::print("XMM3={:x} {:x}\n", cpu.get(XMM::XMM3).hi, cpu.get(XMM::XMM3).lo);
    fmt::print("XMM4={:x} {:x}\n", cpu.get(XMM::XMM4).hi, cpu.get(XMM::XMM4).lo);

    return 0;
}