#include "x64/compiler/assembler.h"
#include "x64/compiler/executablememoryallocator.h"
#include "x64/compiler/jit.h"
#include "x64/disassembler/zydiswrapper.h"
#include "x64/instructions/basicblock.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "verify.h"

using namespace x64;

static BasicBlock create(Cpu* cpu) {
    std::vector<X64Instruction> instructions;
    auto dst = M8 {
        Segment::DS,
        Encoding64 { R64::RDI, R64::ZERO, 1, 0 }
    };
    instructions.push_back(X64Instruction::make(0x0, Insn::REP_STOS_M8_R8, 1, dst, R8::AL));
    instructions.push_back(X64Instruction::make(0x1, Insn::JCC, 1, Cond::E, (u64)0xaaaa));
    return cpu->createBasicBlock(instructions.data(), instructions.size());
}

int main() {
    auto addressSpace = AddressSpace::tryCreate(1);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);
    auto rw = BitFlags<PROT>(PROT::READ, PROT::WRITE);
    auto flags = BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE);
    auto maybe_dst = mmu.mmap(0x0, 0x1000, rw, flags);
    if(!maybe_dst) return 1;
    u64 dst = maybe_dst.value();
    Cpu cpu(mmu);

    static constexpr u64 MAGIC = 0x123456789ABCDEF0;
    static constexpr u64 KEY   = 0x4141414141414141;

    {
        mmu.write64(Ptr64{dst}, MAGIC);
        Cpu::State state;
        state.regs.set(R64::RDI, dst);
        state.regs.set(R64::RCX, 8);
        state.regs.set(R8::AL, 0x41);
        cpu.load(state);

        auto bb = create(&cpu);
        cpu.exec(bb);
        cpu.save(&state);

        u64 value = mmu.read64(Ptr64{dst});
        u64 newdst = state.regs.get(R64::RDI);
        if(value != KEY) return 1;
        if(newdst != dst + 8) return 1;
    }

    {
        mmu.write64(Ptr64{dst}, MAGIC);
        Cpu::State state;
        state.regs.set(R64::RDI, dst);
        state.regs.set(R64::RCX, 8);
        state.regs.set(R8::AL, 0x41);
        cpu.load(state);

        auto bb = create(&cpu);
        auto jit = Jit::tryCreate();
        if(!jit) return 1;
        auto* jbb = jit->tryCompile(bb, nullptr);
        if(!jbb) return 1;

        u64 ticks { 0 };
        std::array<u64, 0x100> basicBlockData;
        std::fill(basicBlockData.begin(), basicBlockData.end(), 0);
        void* basicBlockPtr = &basicBlockData;
        std::array<u64, 0x100> jitBasicBlockData;
        std::fill(jitBasicBlockData.begin(), jitBasicBlockData.end(), 0);
        jit->exec(&cpu, &mmu, (NativeExecPtr)jbb->callEntrypoint(), &ticks, &basicBlockPtr, &jitBasicBlockData);
        cpu.exec(bb);
        cpu.save(&state);

        u64 value = mmu.read64(Ptr64{dst});
        u64 newdst = state.regs.get(R64::RDI);
        if(value != KEY) return 1;
        if(newdst != dst + 8) return 1;
    }

}