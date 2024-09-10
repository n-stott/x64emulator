#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
u32 runShr32Native(u32 val, u8, x64::Flags* flags) {
    u64 rflags = 0;
    asm volatile("shr %1, %0" : "+r" (val) : "c"(count));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return val;
}

u32 runShr32Virtual(u32 val, u8 count, x64::Flags* flags) {
    return x64::CpuImpl::shr32(val, count, flags);
}

template<u8 count>
int compareShr32(u32 val) {
    x64::Flags nativeFlags;
    u32 nativeShr = runShr32Native<count>(val, count%32, &nativeFlags);

    x64::Flags virtFlags;
    u32 virtShr = runShr32Virtual(val, count%32, &virtFlags);

    if(count == 0) {
        if(virtShr == nativeShr) return 0;
    } else  if(count == 1) {
        if(virtShr == nativeShr
        && virtFlags.carry == nativeFlags.carry
        && virtFlags.overflow == nativeFlags.overflow) return 0;    
    } else {
        if(virtShr == nativeShr
        && virtFlags.carry == nativeFlags.carry) return 0;
    }

    fmt::print(stderr, "shr32 {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : rol={:#x} carry={} overflow={}\n",
                        nativeShr, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: rol={:#x} carry={} overflow={}\n",
                        virtShr, virtFlags.carry, virtFlags.overflow);
    return 1;
}

template<int hi>
int loopCompareShr32(u32 val) {
    int rc = 0;
    rc = rc | compareShr32<(hi << 4) | 0x0>(val);
    rc = rc | compareShr32<(hi << 4) | 0x1>(val);
    rc = rc | compareShr32<(hi << 4) | 0x2>(val);
    rc = rc | compareShr32<(hi << 4) | 0x3>(val);
    rc = rc | compareShr32<(hi << 4) | 0x4>(val);
    rc = rc | compareShr32<(hi << 4) | 0x5>(val);
    rc = rc | compareShr32<(hi << 4) | 0x6>(val);
    rc = rc | compareShr32<(hi << 4) | 0x7>(val);
    rc = rc | compareShr32<(hi << 4) | 0x8>(val);
    rc = rc | compareShr32<(hi << 4) | 0x9>(val);
    rc = rc | compareShr32<(hi << 4) | 0xa>(val);
    rc = rc | compareShr32<(hi << 4) | 0xb>(val);
    rc = rc | compareShr32<(hi << 4) | 0xc>(val);
    rc = rc | compareShr32<(hi << 4) | 0xd>(val);
    rc = rc | compareShr32<(hi << 4) | 0xe>(val);
    rc = rc | compareShr32<(hi << 4) | 0xf>(val);
    return rc;
}

int loopCompareShr32(u32 val) {
    int rc = 0;
    rc = rc | loopCompareShr32<0x0>(val);
    rc = rc | loopCompareShr32<0x1>(val);
    rc = rc | loopCompareShr32<0x2>(val);
    rc = rc | loopCompareShr32<0x3>(val);
    rc = rc | loopCompareShr32<0x4>(val);
    rc = rc | loopCompareShr32<0x5>(val);
    rc = rc | loopCompareShr32<0x6>(val);
    rc = rc | loopCompareShr32<0x7>(val);
    rc = rc | loopCompareShr32<0x8>(val);
    rc = rc | loopCompareShr32<0x9>(val);
    rc = rc | loopCompareShr32<0xa>(val);
    rc = rc | loopCompareShr32<0xb>(val);
    rc = rc | loopCompareShr32<0xc>(val);
    rc = rc | loopCompareShr32<0xd>(val);
    rc = rc | loopCompareShr32<0xe>(val);
    rc = rc | loopCompareShr32<0xf>(val);
    return rc;
}

int main() {
    int rc = 0;
    for(u32 val = 0; val <= 0xFFFF; ++val) {
        rc = rc | loopCompareShr32(val);
    }
    return rc;
}