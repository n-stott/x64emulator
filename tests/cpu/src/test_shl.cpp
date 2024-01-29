#include "utils/utils.h"
#include "interpreter/cpuimpl.h"
#include "interpreter/flags.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
u32 runShl32Native(u32 val, u8, x64::Flags* flags) {
    u64 rflags = 0;
    asm volatile("shl %1, %0" : "+r" (val) : "c"(count));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return val;
}

u32 runShl32Virtual(u32 val, u8 count, x64::Flags* flags) {
    return x64::CpuImpl::shl32(val, count, flags);
}

template<u8 count>
int compareShl32(u32 val) {
    x64::Flags nativeFlags;
    u32 nativeShl = runShl32Native<count>(val, count, &nativeFlags);

    x64::Flags virtFlags;
    u32 virtShl = runShl32Virtual(val, count%32, &virtFlags);

    if(virtShl == nativeShl
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.overflow == nativeFlags.overflow) return 0;

    fmt::print(stderr, "shl32 {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : rol={:#x} carry={} overflow={}\n",
                        nativeShl, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: rol={:#x} carry={} overflow={}\n",
                        virtShl, virtFlags.carry, virtFlags.overflow);
    return 1;
}

template<int hi>
int loopCompareShl32(u32 val) {
    int rc = 0;
    rc = rc | compareShl32<(hi << 4) | 0x0>(val);
    rc = rc | compareShl32<(hi << 4) | 0x1>(val);
    rc = rc | compareShl32<(hi << 4) | 0x2>(val);
    rc = rc | compareShl32<(hi << 4) | 0x3>(val);
    rc = rc | compareShl32<(hi << 4) | 0x4>(val);
    rc = rc | compareShl32<(hi << 4) | 0x5>(val);
    rc = rc | compareShl32<(hi << 4) | 0x6>(val);
    rc = rc | compareShl32<(hi << 4) | 0x7>(val);
    rc = rc | compareShl32<(hi << 4) | 0x8>(val);
    rc = rc | compareShl32<(hi << 4) | 0x9>(val);
    rc = rc | compareShl32<(hi << 4) | 0xa>(val);
    rc = rc | compareShl32<(hi << 4) | 0xb>(val);
    rc = rc | compareShl32<(hi << 4) | 0xc>(val);
    rc = rc | compareShl32<(hi << 4) | 0xd>(val);
    rc = rc | compareShl32<(hi << 4) | 0xe>(val);
    rc = rc | compareShl32<(hi << 4) | 0xf>(val);
    return rc;
}

int loopCompareShl32(u32 val) {
    int rc = 0;
    rc = rc | loopCompareShl32<0x0>(val);
    rc = rc | loopCompareShl32<0x1>(val);
    rc = rc | loopCompareShl32<0x2>(val);
    rc = rc | loopCompareShl32<0x3>(val);
    rc = rc | loopCompareShl32<0x4>(val);
    rc = rc | loopCompareShl32<0x5>(val);
    rc = rc | loopCompareShl32<0x6>(val);
    rc = rc | loopCompareShl32<0x7>(val);
    rc = rc | loopCompareShl32<0x8>(val);
    rc = rc | loopCompareShl32<0x9>(val);
    rc = rc | loopCompareShl32<0xa>(val);
    rc = rc | loopCompareShl32<0xb>(val);
    rc = rc | loopCompareShl32<0xc>(val);
    rc = rc | loopCompareShl32<0xd>(val);
    rc = rc | loopCompareShl32<0xe>(val);
    rc = rc | loopCompareShl32<0xf>(val);
    return rc;
}

int main() {
    int rc = 0;
    for(u32 val = 0; val <= 0xFFFF; ++val) {
        rc = rc | loopCompareShl32(val);
    }
    return rc;
}