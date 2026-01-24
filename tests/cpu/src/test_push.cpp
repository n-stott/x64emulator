#include "x64/cpu.h"
#include "x64/mmu.h"
#include <cstdio>

int emulated() {
    using namespace x64;
    auto addressSpace = AddressSpace::tryCreate(1);
    if(!addressSpace) return 1;
    Mmu mmu(*addressSpace);
    Cpu cpu(mmu);

    auto stack = mmu.mmap(0x0, 0x1000, BitFlags<PROT>(PROT::READ, PROT::WRITE), BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE));
    if(!stack) {
        puts("mmap failed");
        return 1;
    }

    cpu.set(R64::RSP, stack.value() + 0x800);

    cpu.push8(0xFF);
    cpu.push16(0xFFFF);
    cpu.push32(0xFFFFFFFF);
    cpu.push64(0xFFFFFFFFFFFFFFFF);

    u64 v64 = cpu.pop64();
    u64 v32 = cpu.pop64();
    u64 v16 = cpu.pop64();
    u64 v8 = cpu.pop64();

    u64 expected = (u64)(-1);

    if(v64 != expected) {
        return 1;
    }
    if(v32 != expected) {
        return 1;
    }
    if(v16 != expected) {
        return 1;
    }
    if(v8 != expected) {
        return 1;
    }

    return 0;
}

int native() {
    
    u64 v8 = 0;
    const i8 val8 = (i8)0xFE;
    asm volatile("push %1;"
        "pop %0": "+r"(v8) : "i"(val8));
    u64 expected8 = (u64)0xFFFFFFFFFFFFFFFE;
    if(v8 != expected8) {
        return 1;
    }

    u64 v16 = 0;
    const i16 val16 = (i16)0xFEDC;
    asm volatile("push %1;"
                 "pop %0": "+r"(v16) : "i"(val16));
    u64 expected16 = (u64)0xFFFFFFFFFFFFFEDC;
    if(v16 != expected16) {
        return 1;
    }

    u64 v32 = 0;
    const i32 val32 = (i32)0xFEDCFEDC;
    asm volatile("push %1;"
                 "pop %0": "+r"(v32) : "i"(val32));
    u64 expected32 = (u64)0xFFFFFFFFFEDCFEDC;
    if(v32 != expected32) {
        return 1;
    }

    return 0;
}

// int emulatedStackSize16() {
//     using namespace x64;
//     auto mmu = Mmu::tryCreate(1);
//     Cpu cpu(mmu);

//     u64 stack = mmu.mmap(0x0, 0x1000, BitFlags<PROT>(PROT::READ, PROT::WRITE), BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE));
//     if(stack == (u64)-1) {
//         puts("mmap failed");
//         return 1;
//     }

//     cpu.set(R64::RSP, stack + 0x800);

//     u64 rspBefore = 0;
//     u64 rspAfter = 0;

//     rspBefore = cpu.get(R64::RSP);
//     cpu.push16(0xFFEE);
//     rspAfter = cpu.get(R64::RSP);

//     if(rspAfter > rspBefore) {
//         return 1;
//     }
//     if(rspAfter + 2 != rspBefore) {
//         return 1;
//     }

//     return 0;
// }


// int nativeStackSize16() {
//     u16 v16 = 0;
//     const i16 val16 = (i16)0xFEDC;

//     u64 rspBefore = 0;
//     u64 rspAfter = 0;

//     asm volatile(
//         "mov %%rsp, %0;"
//         "pushw %3;"
//         "mov %%rsp, %1;"
//         "popw %2": "+r"(rspBefore), "+r"(rspAfter), "+r"(v16) : "i"(val16));

//     if(rspAfter > rspBefore) {
//         return 1;
//     }
//     if(rspAfter + 2 != rspBefore) {
//         return 1;
//     }

//     return 0;
// }

int nativeStackSize32() {
    u64 v32 = 0;
    const i32 val32 = (i32)0xFEDCFEDC;

    u64 rspBefore = 0;
    u64 rspAfter = 0;

    asm volatile(
        "mov %%rsp, %0;"
        "push %3;"
        "mov %%rsp, %1;"
        "pop %2": "+r"(rspBefore), "+r"(rspAfter), "+r"(v32) : "i"(val32));

    if(rspAfter > rspBefore) {
        return 1;
    }
    if(rspAfter + 8 != rspBefore) {
        return 1;
    }

    return 0;
}

int main() {
    int ret = emulated();
    if(ret) return ret;

    ret = native();
    if(ret) return ret;

    // ret = emulatedStackSize16();
    // if(ret) return ret;

    // ret = nativeStackSize16();
    // if(ret) return ret;

    ret = nativeStackSize32();
    if(ret) return ret;

    return 0;
}