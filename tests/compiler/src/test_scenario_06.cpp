#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include <sys/mman.h>

int main(int argc, char**) {
    using namespace x64;
    Mmu mmu;
    Cpu cpu(mmu);

    std::array<X64Instruction, 31> instructions {{
        // paddsw    mm2,mm3
        X64Instruction::make(0x0, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM2, MMXM64{true, MMX::MM3, {}}),
        // mov       mm3,mm7
        X64Instruction::make(0x1, Insn::MOV_MMX_MMX, 1, MMX::MM3, MMX::MM7),
        // mov       mm5,mm7
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM5, MMX::MM7),
        // paddsw    mm3,mm0
        X64Instruction::make(0x3, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM3, MMXM64{true, MMX::MM0, {}}),
        // paddsw    mm5,mm1
        X64Instruction::make(0x4, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM5, MMXM64{true, MMX::MM1, {}}),
        // paddsw    mm7,mm2
        X64Instruction::make(0x5, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM7, MMXM64{true, MMX::MM2, {}}),
        // paddsw    mm0,mm6
        X64Instruction::make(0x6, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM0, MMXM64{true, MMX::MM6, {}}),
        // paddsw    mm1,mm6
        X64Instruction::make(0x7, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM1, MMXM64{true, MMX::MM6, {}}),
        // paddsw    mm2,mm6
        X64Instruction::make(0x8, Insn::PADDSW_MMX_MMXM64, 1, MMX::MM2, MMXM64{true, MMX::MM6, {}}),
        // packuswb  mm0,mm1
        X64Instruction::make(0x9, Insn::PACKUSWB_MMX_MMXM64, 1, MMX::MM0, MMXM64{true, MMX::MM1, {}}),
        // packuswb  mm3,mm5
        X64Instruction::make(0xa, Insn::PACKUSWB_MMX_MMXM64, 1, MMX::MM3, MMXM64{true, MMX::MM5, {}}),
        // packuswb  mm2,mm2
        X64Instruction::make(0xb, Insn::PACKUSWB_MMX_MMXM64, 1, MMX::MM2, MMXM64{true, MMX::MM2, {}}),
        // mov       mm1,mm0
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM1, MMX::MM0),
        // packuswb  mm7,mm7
        X64Instruction::make(0xb, Insn::PACKUSWB_MMX_MMXM64, 1, MMX::MM7, MMXM64{true, MMX::MM7, {}}),
        // punpcklbw mm0,mm3
        X64Instruction::make(0xb, Insn::PUNPCKLBW_MMX_MMXM32, 1, MMX::MM0, MMXM32{true, MMX::MM3, {}}),
        // punpckhbw mm1,mm3
        X64Instruction::make(0xb, Insn::PUNPCKHBW_MMX_MMXM64, 1, MMX::MM1, MMXM64{true, MMX::MM3, {}}),
        // punpcklbw mm2,mm7
        X64Instruction::make(0xb, Insn::PUNPCKLBW_MMX_MMXM32, 1, MMX::MM2, MMXM32{true, MMX::MM7, {}}),
        // pcmpeqd   mm3,mm3
        X64Instruction::make(0xb, Insn::PCMPEQD_MMX_MMXM64, 1, MMX::MM3, MMXM64{true, MMX::MM3, {}}),
        // mov       mm5,mm0
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM5, MMX::MM0),
        // mov       mm6,mm1
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM6, MMX::MM1),
        // punpckhbw mm5,mm2
        X64Instruction::make(0xb, Insn::PUNPCKHBW_MMX_MMXM64, 1, MMX::MM5, MMXM64{true, MMX::MM2, {}}),
        // punpcklbw mm0,mm2
        X64Instruction::make(0xb, Insn::PUNPCKLBW_MMX_MMXM32, 1, MMX::MM0, MMXM32{true, MMX::MM2, {}}),
        // punpckhbw mm6,mm3
        X64Instruction::make(0xb, Insn::PUNPCKHBW_MMX_MMXM64, 1, MMX::MM6, MMXM64{true, MMX::MM3, {}}),
        // punpcklbw mm1,mm3
        X64Instruction::make(0xb, Insn::PUNPCKLBW_MMX_MMXM32, 1, MMX::MM1, MMXM32{true, MMX::MM3, {}}),
        // mov       mm2,mm0
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM2, MMX::MM0),
        // mov       mm3,mm5
        X64Instruction::make(0x2, Insn::MOV_MMX_MMX, 1, MMX::MM3, MMX::MM5),
        // punpcklwd mm0,mm1
        X64Instruction::make(0xb, Insn::PUNPCKLWD_MMX_MMXM32, 1, MMX::MM0, MMXM32{true, MMX::MM1, {}}),
        // punpckhwd mm2,mm1
        X64Instruction::make(0xb, Insn::PUNPCKHWD_MMX_MMXM64, 1, MMX::MM2, MMXM64{true, MMX::MM1, {}}),
        // punpcklwd mm5,mm6
        X64Instruction::make(0xb, Insn::PUNPCKLWD_MMX_MMXM32, 1, MMX::MM5, MMXM32{true, MMX::MM6, {}}),
        // punpckhwd mm3,mm6
        X64Instruction::make(0xb, Insn::PUNPCKHWD_MMX_MMXM64, 1, MMX::MM3, MMXM64{true, MMX::MM6, {}}),
        X64Instruction::make(0xc, Insn::JMP_U32, 1, (u32)0x0),
    }};

    auto bb = cpu.createBasicBlock(instructions.data(), instructions.size());

    cpu.set(MMX::MM0, 0x0101010101010101);
    cpu.set(MMX::MM1, 0x1213121312131213);
    cpu.set(MMX::MM2, 0x2426242624262426);
    cpu.set(MMX::MM3, 0x3639363936393639);
    cpu.set(MMX::MM4, 0x1234123443214321);
    cpu.set(MMX::MM5, 0x2468246886428642);
    cpu.set(MMX::MM6, 0x3690369009630963);
    cpu.set(MMX::MM7, 0xabcdabcddcbadcba);

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

    fmt::print("MM0={:x}\n", cpu.get(MMX::MM0));
    fmt::print("MM1={:x}\n", cpu.get(MMX::MM1));
    fmt::print("MM2={:x}\n", cpu.get(MMX::MM2));
    fmt::print("MM3={:x}\n", cpu.get(MMX::MM3));
    fmt::print("MM4={:x}\n", cpu.get(MMX::MM4));
    fmt::print("MM5={:x}\n", cpu.get(MMX::MM5));
    fmt::print("MM6={:x}\n", cpu.get(MMX::MM6));
    fmt::print("MM7={:x}\n", cpu.get(MMX::MM7));

    return 0;
}