#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

u8 runSbb8Native(u8 lhs, u8 rhs, x64::Flags* flags, bool carry) {
    if(carry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("sbb %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("sbb %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    }
    return lhs;
}

u8 runSbb8Virtual(u8 lhs, u8 rhs, x64::Flags* flags, bool carry) {
    flags->carry = carry;
    return x64::CpuImpl::sbb8(lhs, rhs, flags);
}

int compareSbb8(u8 lhs, u8 rhs, bool carry) {
    x64::Flags nativeFlags;
    u8 nativeDiff = runSbb8Native(lhs, rhs, &nativeFlags, carry);

    x64::Flags virtFlags;
    u8 virtDiff = runSbb8Virtual(lhs, rhs, &virtFlags, carry);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity() == nativeFlags.parity()) return 0;

    fmt::print(stderr, "sbb8 {:#x} {:#x} carry={} failed\n", lhs, rhs, carry);
    fmt::print(stderr, "native : diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        nativeDiff, nativeFlags.carry, nativeFlags.zero, nativeFlags.overflow, nativeFlags.sign, nativeFlags.parity());
    fmt::print(stderr, "virtual: diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        virtDiff, virtFlags.carry, virtFlags.zero, virtFlags.overflow, virtFlags.sign, virtFlags.parity());
    return 1;
}


u64 runSbb64Native(u64 lhs, u64 rhs, x64::Flags* flags, bool carry) {
    if(carry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("sbb %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("sbb %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    }
    return lhs;
}

u64 runSbb64Virtual(u64 lhs, u64 rhs, x64::Flags* flags, bool carry) {
    flags->carry = carry;
    return x64::CpuImpl::sbb64(lhs, rhs, flags);
}

int compareSbb64(u64 lhs, u64 rhs, bool carry) {
    x64::Flags nativeFlags;
    u64 nativeDiff = runSbb64Native(lhs, rhs, &nativeFlags, carry);

    x64::Flags virtFlags;
    u64 virtDiff = runSbb64Virtual(lhs, rhs, &virtFlags, carry);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity() == nativeFlags.parity()) return 0;

    fmt::print(stderr, "sbb64 {:#x} {:#x} carry={} failed\n", lhs, rhs, carry);
    fmt::print(stderr, "native : diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        nativeDiff, nativeFlags.carry, nativeFlags.zero, nativeFlags.overflow, nativeFlags.sign, nativeFlags.parity());
    fmt::print(stderr, "virtual: diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        virtDiff, virtFlags.carry, virtFlags.zero, virtFlags.overflow, virtFlags.sign, virtFlags.parity());
    return 1;
}


int main() {
    int rc = 0;
    for(u16 lhs = 0; lhs < 256; ++lhs) {
        for(u16 rhs = 0; rhs < 256; ++rhs) {
            rc = rc | compareSbb8((u8)lhs, (u8)rhs, false);
            rc = rc | compareSbb8((u8)lhs, (u8)rhs, true);
        }
    }
    rc |= compareSbb64(0, 0, false);
    rc |= compareSbb64(0, 0, true);
    rc |= compareSbb64((u64)(-1), 0, false);
    rc |= compareSbb64((u64)(-1), 0, true);
    rc |= compareSbb64(0, (u64)(-1), false);
    rc |= compareSbb64(0, (u64)(-1), true);
    rc |= compareSbb64(2, (u64)(-1), false);
    rc |= compareSbb64(2, (u64)(-1), true);
    rc |= compareSbb64((u64)(-1), 2, false);
    rc |= compareSbb64((u64)(-1), 2, true);
    rc |= compareSbb64(10, 10, false);
    rc |= compareSbb64(10, 10, true);
    rc |= compareSbb64(10, 11, false);
    rc |= compareSbb64(10, 11, true);
    rc |= compareSbb64(11, 10, false);
    rc |= compareSbb64(11, 10, true);
    return rc;
}