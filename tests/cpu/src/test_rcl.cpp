#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
u32 runRcl32Native(u32 val, u8, x64::Flags* flags, bool initialCarry) {
    if(initialCarry) {
        u64 rflags = 0;
        asm volatile("stc");
        asm volatile("rcl %1, %0" : "+r" (val) : "i"(count));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
        return val;
    } else {
        u64 rflags = 0;
        asm volatile("clc");
        asm volatile("rcl %1, %0" : "+r" (val) : "i"(count));
        asm volatile("pushf");
        asm volatile("pop %0" : "=r" (rflags));
        *flags = fromRflags(rflags);
        return val;
    }
}

u32 runRcl32Virtual(u32 val, u8 count, x64::Flags* flags) {
    return x64::CpuImpl::rcl32(val, count, flags);
}

template<u8 count>
int compareRcl32(u32 val, bool initialCarry) {
    x64::Flags nativeFlags;
    u32 nativeRcl = runRcl32Native<count>(val, count, &nativeFlags, initialCarry);

    x64::Flags virtFlags;
    virtFlags.carry = initialCarry;
    u32 virtRcl = runRcl32Virtual(val, count, &virtFlags);

    if(count == 0) {
        if(virtRcl == nativeRcl) return 0;
    } else  if(count == 1) {
        if(virtRcl == nativeRcl
        && virtFlags.carry == nativeFlags.carry
        && virtFlags.overflow == nativeFlags.overflow) return 0;    
    } else {
        if(virtRcl == nativeRcl
        && virtFlags.carry == nativeFlags.carry) return 0;
    }

    fmt::print(stderr, "rcl32 {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : rcl={:#x} carry={} overflow={}\n",
                        nativeRcl, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: rcl={:#x} carry={} overflow={}\n",
                        virtRcl, virtFlags.carry, virtFlags.overflow);
    return 1;
}

template<int hi>
int loopCompareRcl32(u32 val, bool initialCarry) {
    int rc = 0;
    rc = rc | compareRcl32<(hi << 4) | 0x0>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x1>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x2>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x3>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x4>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x5>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x6>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x7>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x8>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0x9>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xa>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xb>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xc>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xd>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xe>(val, initialCarry);
    rc = rc | compareRcl32<(hi << 4) | 0xf>(val, initialCarry);
    return rc;
}

int loopCompareRcl32(u32 val, bool initialCarry) {
    int rc = 0;
    rc = rc | loopCompareRcl32<0x0>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x1>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x2>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x3>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x4>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x5>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x6>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x7>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x8>(val, initialCarry);
    rc = rc | loopCompareRcl32<0x9>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xa>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xb>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xc>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xd>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xe>(val, initialCarry);
    rc = rc | loopCompareRcl32<0xf>(val, initialCarry);
    return rc;
}

int main() {
    int rc = 0;
    for(u32 val = 0; val <= 0xFFFF; ++val) {
        rc = rc | loopCompareRcl32(val, false);
        rc = rc | loopCompareRcl32(val, true);
    }
    return rc;
}