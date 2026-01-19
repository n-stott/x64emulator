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
    auto dst = M32 {
        Segment::DS,
        Encoding64 { R64::RDI, R64::ZERO, 1, 0 }
    };
    auto src = M32 {
        Segment::DS,
        Encoding64 { R64::RSI, R64::ZERO, 1, 0 }
    };
    instructions.push_back(X64Instruction::make(0x0, Insn::REP_MOVS_M32_M32, 1, dst, src));
    instructions.push_back(X64Instruction::make(0x1, Insn::JCC, 1, Cond::E, (u64)0xaaaa));
    return cpu->createBasicBlock(instructions.data(), instructions.size());
}

int main() {
    auto mmu = Mmu::tryCreateWithAddressSpace(0x1000);
    if(!mmu) return 1;
    auto rw = BitFlags<PROT>(PROT::READ, PROT::WRITE);
    auto flags = BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE);
    auto maybe_dst = mmu->mmap(0x0, 0x1000, rw, flags);
    auto maybe_src = mmu->mmap(0x0, 0x1000, rw, flags);
    if(!maybe_dst) return 1;
    if(!maybe_src) return 1;
    u64 dst = maybe_dst.value();
    u64 src = maybe_src.value();
    Cpu cpu(*mmu);

    static constexpr u64 MAGIC = 0x12345678;

    {
        Cpu::State state;
        state.regs.set(R64::RDI, dst);
        state.regs.set(R64::RSI, src);
        state.regs.set(R64::RCX, 2);
        cpu.load(state);
        mmu->write64(Ptr64{src}, MAGIC);

        auto bb = create(&cpu);
        cpu.exec(bb);
        cpu.save(&state);

        u64 value = mmu->read64(Ptr64{dst});
        u64 newdst = state.regs.get(R64::RDI);
        u64 newsrc = state.regs.get(R64::RSI);
        if(value != MAGIC) return 1;
        if(newdst != dst + 8) return 1;
        if(newsrc != src + 8) return 1;
    }

    {
        Cpu::State state;
        state.regs.set(R64::RDI, dst);
        state.regs.set(R64::RSI, src);
        state.regs.set(R64::RCX, 2);
        cpu.load(state);
        mmu->write64(Ptr64{src}, MAGIC);

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
        jit->exec(&cpu, mmu.get(), (NativeExecPtr)jbb->executableMemory(), &ticks, &basicBlockPtr, &jitBasicBlockData);
        cpu.exec(bb);
        cpu.save(&state);

        u64 value = mmu->read64(Ptr64{dst});
        u64 newdst = state.regs.get(R64::RDI);
        u64 newsrc = state.regs.get(R64::RSI);
        if(value != MAGIC) return 1;
        if(newdst != dst + 8) return 1;
        if(newsrc != src + 8) return 1;
    }

}