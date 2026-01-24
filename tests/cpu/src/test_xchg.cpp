#include "x64/mmu.h"
#include "x64/cpu.h"

int testXchgRegReg() {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(16);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);

    Cpu cpu(mmu);

    Cpu::State state;
    cpu.save(&state);
    state.regs.set(R64::RAX, 0x1234);
    state.regs.set(R64::RBX, 0x5678);
    cpu.load(state);

    X64Instruction xchgRaxRbx = X64Instruction::make(0x0, Insn::XCHG_RM64_R64, 1, RM64{true, R64::RAX, {}}, R64::RBX);
    cpu.exec(xchgRaxRbx);

    Cpu::State state2;
    cpu.save(&state2);
    if(state2.regs.get(R64::RAX) != 0x5678) return 1;
    if(state2.regs.get(R64::RBX) != 0x1234) return 1;

    return 0;
}

int testXchgMemReg() {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(16);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);

    auto base = mmu.mmap(0x1000, 0x1000, BitFlags<PROT>(PROT::READ, PROT::WRITE), BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE, MAP::FIXED));
    if(base != 0x1000) return 1;

    Ptr64 ptr { base.value() };
    mmu.write64(ptr, 0x1234);
    
    Cpu cpu(mmu);

    Cpu::State state;
    cpu.save(&state);
    state.regs.set(R64::RAX, 0x5678);
    cpu.load(state);

    M64 mem { Segment::DS, Encoding64{R64::ZERO, R64::ZERO, 1, (i32)base.value()} };
    X64Instruction xchgMemRax = X64Instruction::make(0x0, Insn::XCHG_RM64_R64, 1, RM64{false, R64::ZERO, mem}, R64::RAX);
    cpu.exec(xchgMemRax);

    Cpu::State state2;
    cpu.save(&state2);
    if(state2.regs.get(R64::RAX) != 0x1234) {
        return 1;
    }
    if(mmu.read64(ptr) != 0x5678) {
        return 1;
    }

    return 0;
}

int main() {
    int rc = 0;
    rc |= testXchgRegReg();
    rc |= testXchgMemReg();
    return rc;
}