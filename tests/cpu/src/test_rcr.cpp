#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
u32 runRcr32Native(u32 val, u8, x64::Flags* flags, bool initialCarry) {
    if(initialCarry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("rcr %1, %0" : "+r" (val) : "i"(count));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
        return val;
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("rcr %1, %0" : "+r" (val) : "i"(count));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
        return val;
    }
}

u32 runRcr32Virtual(u32 val, u8 count, x64::Flags* flags) {
    return x64::CpuImpl::rcr32(val, count, flags);
}

template<u8 count>
int compareRcr32(u32 val, bool initialCarry) {
    x64::Flags nativeFlags;
    u32 nativeRcr = runRcr32Native<count>(val, count, &nativeFlags, initialCarry);

    x64::Flags virtFlags;
    virtFlags.carry = initialCarry;
    u32 virtRcr = runRcr32Virtual(val, count, &virtFlags);

    if(count == 0) {
        if(virtRcr == nativeRcr) return 0;
    } else  if(count == 1) {
        if(virtRcr == nativeRcr
        && virtFlags.carry == nativeFlags.carry
        && virtFlags.overflow == nativeFlags.overflow) return 0;    
    } else {
        if(virtRcr == nativeRcr
        && virtFlags.carry == nativeFlags.carry) return 0;
    }

    fmt::print(stderr, "rcr32 {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : rcr={:#x} carry={} overflow={}\n",
                        nativeRcr, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: rcr={:#x} carry={} overflow={}\n",
                        virtRcr, virtFlags.carry, virtFlags.overflow);
    return 1;
}

template<int hi>
int loopCompareRcr32(u32 val, bool initialCarry) {
    int rc = 0;
    rc = rc | compareRcr32<(hi << 4) | 0x0>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x1>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x2>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x3>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x4>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x5>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x6>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x7>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x8>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0x9>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xa>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xb>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xc>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xd>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xe>(val, initialCarry);
    rc = rc | compareRcr32<(hi << 4) | 0xf>(val, initialCarry);
    return rc;
}

int loopCompareRcr32(u32 val, bool initialCarry) {
    int rc = 0;
    rc = rc | loopCompareRcr32<0x0>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x1>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x2>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x3>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x4>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x5>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x6>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x7>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x8>(val, initialCarry);
    rc = rc | loopCompareRcr32<0x9>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xa>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xb>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xc>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xd>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xe>(val, initialCarry);
    rc = rc | loopCompareRcr32<0xf>(val, initialCarry);
    return rc;
}

int main() {
    int rc = 0;
    for(u32 val = 0; val <= 0xFFFF; ++val) {
        rc = rc | loopCompareRcr32(val, false);
        rc = rc | loopCompareRcr32(val, true);
    }
    return rc;
}