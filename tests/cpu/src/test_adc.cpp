#include "utils/utils.h"
#include "interpreter/cpuimpl.h"
#include "interpreter/flags.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

u8 runAdc8Native(u8 lhs, u8 rhs, x64::Flags* flags, bool carry) {
    if(carry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("adc %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("adc %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    }
    return lhs;
}

u8 runAdc8Virtual(u8 lhs, u8 rhs, x64::Flags* flags, bool carry) {
    flags->carry = carry;
    return x64::Impl::adc8(lhs, rhs, flags);
}

int compareAdc8(u8 lhs, u8 rhs, bool carry) {
    x64::Flags nativeFlags;
    u8 nativeDiff = runAdc8Native(lhs, rhs, &nativeFlags, carry);

    x64::Flags virtFlags;
    u8 virtDiff = runAdc8Virtual(lhs, rhs, &virtFlags, carry);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity == nativeFlags.parity) return 0;

    fmt::print(stderr, "adc8 {:#x} {:#x} carry={} failed\n", lhs, rhs, carry);
    fmt::print(stderr, "native : diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        nativeDiff, nativeFlags.carry, nativeFlags.zero, nativeFlags.overflow, nativeFlags.sign, nativeFlags.parity);
    fmt::print(stderr, "virtual: diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        virtDiff, virtFlags.carry, virtFlags.zero, virtFlags.overflow, virtFlags.sign, virtFlags.parity);
    return 1;
}


u64 runAdc64Native(u64 lhs, u64 rhs, x64::Flags* flags, bool carry) {
    if(carry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("adc %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("adc %1, %0" : "+r" (lhs) : "r"(rhs));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
    }
    return lhs;
}

u64 runAdc64Virtual(u64 lhs, u64 rhs, x64::Flags* flags, bool carry) {
    flags->carry = carry;
    return x64::Impl::adc64(lhs, rhs, flags);
}

int compareAdc64(u64 lhs, u64 rhs, bool carry) {
    x64::Flags nativeFlags;
    u64 nativeDiff = runAdc64Native(lhs, rhs, &nativeFlags, carry);

    x64::Flags virtFlags;
    u64 virtDiff = runAdc64Virtual(lhs, rhs, &virtFlags, carry);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity == nativeFlags.parity) return 0;

    fmt::print(stderr, "adc64 {:#x} {:#x} carry={} failed\n", lhs, rhs, carry);
    fmt::print(stderr, "native : diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        nativeDiff, nativeFlags.carry, nativeFlags.zero, nativeFlags.overflow, nativeFlags.sign, nativeFlags.parity);
    fmt::print(stderr, "virtual: diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        virtDiff, virtFlags.carry, virtFlags.zero, virtFlags.overflow, virtFlags.sign, virtFlags.parity);
    return 1;
}


int main() {
    int rc = 0;
    for(u16 lhs = 0; lhs < 256; ++lhs) {
        for(u16 rhs = 0; rhs < 256; ++rhs) {
            rc = rc | compareAdc8((u8)lhs, (u8)rhs, false);
            rc = rc | compareAdc8((u8)lhs, (u8)rhs, true);
        }
    }
    rc |= compareAdc64(0, 0, false);
    rc |= compareAdc64(0, 0, true);
    rc |= compareAdc64((u64)(-1), 0, false);
    rc |= compareAdc64((u64)(-1), 0, true);
    rc |= compareAdc64(0, (u64)(-1), false);
    rc |= compareAdc64(0, (u64)(-1), true);
    rc |= compareAdc64(2, (u64)(-1), false);
    rc |= compareAdc64(2, (u64)(-1), true);
    rc |= compareAdc64((u64)(-1), 2, false);
    rc |= compareAdc64((u64)(-1), 2, true);
    rc |= compareAdc64(10, 10, false);
    rc |= compareAdc64(10, 10, true);
    rc |= compareAdc64(10, 11, false);
    rc |= compareAdc64(10, 11, true);
    rc |= compareAdc64(11, 10, false);
    rc |= compareAdc64(11, 10, true);
    return rc;
}