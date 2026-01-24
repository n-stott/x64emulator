#include "x64/cpu.h"
#include "x64/flags.h"
#include "x64/mmu.h"
#include "utils.h"
#include <fmt/format.h>

namespace {
    struct Data {
        u128* m;
        u64* z;
        u64 a;
        u64 d;
        u64 b;
        u64 c;
    };

    void native(Data* data) {
        asm volatile(
            "mov %2, %%rax;"
            "mov %3, %%rbx;"
            "mov %4, %%rcx;"
            "mov %5, %%rdx;"
            "cmpxchg16b %0;"
            "pushf;"
            "pop %1;"
            : "+m"(*data->m), "+m"(*data->z)
            : "m"(data->a), "m"(data->b), "m"(data->c), "m"(data->d)
            : "rax", "rbx", "rcx", "rdx", "cc", "memory"
        );

    }
}


bool test(u128 initial, u128 expected, u128 replacement) {

    u128 nativeResult;
    bool nativeZeroFlag { false };

    {
        u128 m = initial;
        u64 z = 0;
        Data data { &m, &z, expected.lo, expected.hi, replacement.lo, replacement.hi };
        native(&data);
        nativeResult = m;
        nativeZeroFlag = (z & x64::Flags::ZERO_MASK) > 0;
    }

    u128 emulatedResult;
    bool emulatedZeroFlag { false };
    {
        using namespace x64;
        auto addressSpace = AddressSpace::tryCreate(1);
        if(!addressSpace) return false;
        Mmu mmu(*addressSpace);
        auto maybe_base = mmu.mmap(0, 0x1000, BitFlags<PROT>(PROT::READ, PROT::WRITE), BitFlags<MAP>(MAP::ANONYMOUS, MAP::PRIVATE));
        if(!maybe_base) return false;
        u64 base = maybe_base.value();
        Ptr128 ptr { base };
        mmu.write128(ptr, initial);

        Cpu cpu(mmu);
        cpu.set(R64::RDX, expected.hi);
        cpu.set(R64::RAX, expected.lo);
        cpu.set(R64::RCX, replacement.hi);
        cpu.set(R64::RBX, replacement.lo);

        X64Instruction ins = X64Instruction::make(0, Insn::CMPXCHG16B_M128, 1, M128{Segment::UNK, Encoding64{R64::ZERO, R64::ZERO, 1, (i32)base}});

        cpu.execCmpxchg16BM128(ins);

        emulatedResult = mmu.read128(ptr);
        Cpu::State state;
        cpu.save(&state);
        emulatedZeroFlag = state.flags.zero;
    }

    if(nativeResult != emulatedResult) return false;
    if(nativeZeroFlag != emulatedZeroFlag) return false;
    return true;
}


int main() {
    {
        u128 initial { 0, 0 };
        u128 expected { 0, 0 };
        u128 replacement { 1, 2 };
        if(!test(initial, expected, replacement)) return 1;
    }
    {
        u128 initial { 0, 0 };
        u128 expected { 1, 0 };
        u128 replacement { 1, 2 };
        if(!test(initial, expected, replacement)) return 1;
    }
    {
        u128 initial { 0, 0 };
        u128 expected { 0, 1 };
        u128 replacement { 1, 2 };
        if(!test(initial, expected, replacement)) return 1;
    }
    {
        u128 initial { 0, 0 };
        u128 expected { 1, 1 };
        u128 replacement { 2, 2 };
        if(!test(initial, expected, replacement)) return 1;
    }
    return 0;
}