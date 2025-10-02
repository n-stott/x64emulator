#include "x64/mmu.h"

using namespace x64;

int main() {
    auto mmu = Mmu::tryCreate(0x100);
    if(!mmu) return 1;

    BitFlags<PROT> prot;
    prot.add(PROT::READ);

    BitFlags<MAP> flags;
    flags.add(MAP::ANONYMOUS);
    flags.add(MAP::PRIVATE);

    try {
        u64 consa = mmu->memoryConsumptionInMB();
        u64 size = 0x1000000;
        u64 base = mmu->mmap(0x0, size, prot, flags);

        u64 consb = mmu->memoryConsumptionInMB();

        prot.add(PROT::WRITE);
        mmu->mprotect(base + size/4, size/2, prot);

        mmu->munmap(base, size/2);

        u64 consc = mmu->memoryConsumptionInMB();

        mmu->munmap(base+size/2, size/2);

        u64 consd = mmu->memoryConsumptionInMB();

        if(consd != 0) return 1;

        fmt::println("consa {}", consa);
        fmt::println("consb {}", consb);
        fmt::println("consc {}", consc);
        fmt::println("consd {}", consd);

    } catch(...) {
        return 1;
    }

    return 0;
}