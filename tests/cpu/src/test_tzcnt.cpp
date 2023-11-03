#include "utils/utils.h"
#include "interpreter/cpu.h"
#include "interpreter/flags.h"
#include "fmt/core.h"
#include <vector>

struct Result {
    u32 count;
    x64::Flags flags;
};

static x64::Flags fromRflags(u64 rflags) {
    static constexpr u64 CARRY_MASK = 0x1;
    static constexpr u64 PARITY_MASK = 0x4;
    static constexpr u64 ZERO_MASK = 0x40;
    static constexpr u64 SIGN_MASK = 0x80;
    static constexpr u64 OVERFLOW_MASK = 0x800;
    x64::Flags flags;
    flags.carry = rflags & CARRY_MASK;
    flags.parity = rflags & PARITY_MASK;
    flags.zero = rflags & ZERO_MASK;
    flags.sign = rflags & SIGN_MASK;
    flags.overflow = rflags & OVERFLOW_MASK;
    flags.setSure();
    return flags;
}

u64 runTzcnt64Native(u64 value, x64::Flags* flags) {
    u64 count = 0;
    u64 rflags = 0;
    asm volatile("tzcnt %1, %0" : "=r" (count) : "r" (value));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return count;
}

u64 runTzcnt64Virtual(u64 value, x64::Flags* flags) {
    return x64::Cpu::Impl::tzcnt64(value, flags);
}

int compareTzcnt64(u64 value) {
    x64::Flags nativeFlags;
    u64 nativeCount = runTzcnt64Native(value, &nativeFlags);

    x64::Flags virtFlags;
    u64 virtCount = runTzcnt64Virtual(value, &virtFlags);

    if(virtCount == nativeCount
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero) return 0;

    fmt::print(stderr, "tzcnt {:#x} failed\n", value);
    fmt::print(stderr, "native : count={:#x} carry={} zero={}\n", nativeCount, nativeFlags.carry, nativeFlags.zero);
    fmt::print(stderr, "virtual: count={:#x} carry={} zero={}\n", virtCount, virtFlags.carry, virtFlags.zero);
    return 1;
}

int main() {
    std::vector<u64> candidates;
    candidates.push_back(0);
    for(u64 i = 0; i < 64; ++i) {
        candidates.push_back(1ull << i);
    }
    int rc = 0;
    for(u64 value : candidates) {
        rc = rc | compareTzcnt64(value);
    }
    return rc;
}