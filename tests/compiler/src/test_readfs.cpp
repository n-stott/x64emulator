#include "x64/compiler/assembler.h"
#include "x64/compiler/executablememoryallocator.h"
#include "x64/compiler/jit.h"
#include "x64/disassembler/zydiswrapper.h"
#include "x64/instructions/basicblock.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "verify.h"

using namespace x64;

static BasicBlock create(Cpu* cpu, R64 dst, R64 base, R64 index) {
    std::vector<X64Instruction> instructions;
    auto src = M64 {
        Segment::FS,
        Encoding64 {
            base,
            index,
            1,
            0
        }
    };
    instructions.push_back(X64Instruction::make(0x0, Insn::MOV_R64_M64, 1, dst, src));
    instructions.push_back(X64Instruction::make(0x1, Insn::JCC, 1, Cond::E, (u64)0xaaaa));
    return cpu->createBasicBlock(instructions.data(), instructions.size());
}

int main() {
    auto mmu = Mmu::tryCreateWithAddressSpace(0x1000);
    if(!mmu) return 1;
    auto rw = BitFlags<PROT>(PROT::READ, PROT::WRITE);
    auto flags = BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE);
    auto maybe_fsbase = mmu->mmap(0x0, 0x1000, rw, flags);
    if(!maybe_fsbase) return 1;
    auto fsbase = maybe_fsbase.value();
    Cpu cpu(*mmu);

    static constexpr u64 MAGIC = 0x12345678;

    {
        Cpu::State state;
        state.regs.set(R64::RAX, 0);
        state.regs.set(R64::RBX, 0);
        state.regs.set(R64::RCX, 0);
        state.segmentBase[(int)Segment::FS] = fsbase;
        cpu.load(state);
        mmu->write64(Ptr64{fsbase}, MAGIC);

        auto bb = create(&cpu, R64::RAX, R64::RBX, R64::RCX);
        cpu.exec(bb);
        cpu.save(&state);

        if(state.regs.get(R64::RAX) != MAGIC) return 1;
    }

    {
        Cpu::State state;
        state.regs.set(R64::RAX, 0);
        state.regs.set(R64::RBX, 0);
        state.regs.set(R64::RCX, 0);
        state.segmentBase[(int)Segment::FS] = fsbase;
        cpu.load(state);
        mmu->write64(Ptr64{fsbase}, MAGIC);

        auto bb = create(&cpu, R64::RAX, R64::RBX, R64::RCX);
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

        if(state.regs.get(R64::RAX) != MAGIC) return 1;
    }

}