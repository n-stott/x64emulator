#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

u8 runAdd8Native(u8 lhs, u8 rhs, x64::Flags* flags) {
    u64 rflags = 0;
    asm volatile("add %1, %0" : "+r" (lhs) : "r"(rhs));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return lhs;
}

u8 runAdd8Virtual(u8 lhs, u8 rhs, x64::Flags* flags) {
    return x64::CpuImpl::add8(lhs, rhs, flags);
}

int compareAdd8(u8 lhs, u8 rhs) {
    x64::Flags nativeFlags;
    u8 nativeDiff = runAdd8Native(lhs, rhs, &nativeFlags);

    x64::Flags virtFlags;
    u8 virtDiff = runAdd8Virtual(lhs, rhs, &virtFlags);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity == nativeFlags.parity) return 0;

    fmt::print(stderr, "Add8 {:#x} {:#x} failed\n", lhs, rhs);
    fmt::print(stderr, "native : diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        nativeDiff, nativeFlags.carry, nativeFlags.zero, nativeFlags.overflow, nativeFlags.sign, nativeFlags.parity);
    fmt::print(stderr, "virtual: diff={:#x} carry={} zero={} overflow={} sign={} parity={}\n",
                        virtDiff, virtFlags.carry, virtFlags.zero, virtFlags.overflow, virtFlags.sign, virtFlags.parity);
    return 1;
}


u64 runAdd64Native(u64 lhs, u64 rhs, x64::Flags* flags) {
    u64 rflags = 0;
    asm volatile("add %1, %0" : "+r" (lhs) : "r"(rhs));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return lhs;
}

u64 runAdd64Virtual(u64 lhs, u64 rhs, x64::Flags* flags) {
    return x64::CpuImpl::add64(lhs, rhs, flags);
}

int compareAdd64(u64 lhs, u64 rhs) {
    x64::Flags nativeFlags;
    u64 nativeDiff = runAdd64Native(lhs, rhs, &nativeFlags);

    x64::Flags virtFlags;
    u64 virtDiff = runAdd64Virtual(lhs, rhs, &virtFlags);

    if(virtDiff == nativeDiff
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.zero == nativeFlags.zero
    && virtFlags.overflow == nativeFlags.overflow
    && virtFlags.sign == nativeFlags.sign
    && virtFlags.parity == nativeFlags.parity) return 0;

    fmt::print(stderr, "Add64 {:#x} {:#x} failed\n", lhs, rhs);
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
            rc = rc | compareAdd8((u8)lhs, (u8)rhs);
        }
    }
    rc |= compareAdd64(0, 0);
    rc |= compareAdd64((u64)(-1), 0);
    rc |= compareAdd64(0, (u64)(-1));
    rc |= compareAdd64(2, (u64)(-1));
    rc |= compareAdd64((u64)(-1), 2);
    rc |= compareAdd64(10, 10);
    rc |= compareAdd64(10, 11);
    rc |= compareAdd64(11, 10);
    return rc;
}