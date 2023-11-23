#include "utils/utils.h"
#include "interpreter/cpu.h"
#include "interpreter/flags.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
u32 runRor32Native(u32 val, u8, x64::Flags* flags) {
    u64 rflags = 0;
    asm volatile("ror %1, %0" : "+r" (val) : "i"(count));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return val;
}

u32 runRor32Virtual(u32 val, u8 count, x64::Flags* flags) {
    return x64::Cpu::Impl::ror32(val, count, flags);
}

template<u8 count>
int compareRor32(u32 val) {
    x64::Flags nativeFlags;
    u32 nativeRor = runRor32Native<count>(val, count, &nativeFlags);

    x64::Flags virtFlags;
    u32 virtRor = runRor32Virtual(val, count, &virtFlags);

    if(virtRor == nativeRor
    && virtFlags.carry == nativeFlags.carry
    && virtFlags.overflow == nativeFlags.overflow) return 0;

    fmt::print(stderr, "ror32 {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : ror={:#x} carry={} overflow={}\n",
                        nativeRor, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: ror={:#x} carry={} overflow={}\n",
                        virtRor, virtFlags.carry, virtFlags.overflow);
    return 1;
}


template<int hi>
int loopCompareRor32(u32 val) {
    int rc = 0;
    rc = rc | compareRor32<(hi << 4) | 0x0>(val);
    rc = rc | compareRor32<(hi << 4) | 0x1>(val);
    rc = rc | compareRor32<(hi << 4) | 0x2>(val);
    rc = rc | compareRor32<(hi << 4) | 0x3>(val);
    rc = rc | compareRor32<(hi << 4) | 0x4>(val);
    rc = rc | compareRor32<(hi << 4) | 0x5>(val);
    rc = rc | compareRor32<(hi << 4) | 0x6>(val);
    rc = rc | compareRor32<(hi << 4) | 0x7>(val);
    rc = rc | compareRor32<(hi << 4) | 0x8>(val);
    rc = rc | compareRor32<(hi << 4) | 0x9>(val);
    rc = rc | compareRor32<(hi << 4) | 0xa>(val);
    rc = rc | compareRor32<(hi << 4) | 0xb>(val);
    rc = rc | compareRor32<(hi << 4) | 0xc>(val);
    rc = rc | compareRor32<(hi << 4) | 0xd>(val);
    rc = rc | compareRor32<(hi << 4) | 0xe>(val);
    rc = rc | compareRor32<(hi << 4) | 0xf>(val);
    return rc;
}

int loopCompareRor32(u32 val) {
    int rc = 0;
    rc = rc | loopCompareRor32<0x0>(val);
    rc = rc | loopCompareRor32<0x1>(val);
    rc = rc | loopCompareRor32<0x2>(val);
    rc = rc | loopCompareRor32<0x3>(val);
    rc = rc | loopCompareRor32<0x4>(val);
    rc = rc | loopCompareRor32<0x5>(val);
    rc = rc | loopCompareRor32<0x6>(val);
    rc = rc | loopCompareRor32<0x7>(val);
    rc = rc | loopCompareRor32<0x8>(val);
    rc = rc | loopCompareRor32<0x9>(val);
    rc = rc | loopCompareRor32<0xa>(val);
    rc = rc | loopCompareRor32<0xb>(val);
    rc = rc | loopCompareRor32<0xc>(val);
    rc = rc | loopCompareRor32<0xd>(val);
    rc = rc | loopCompareRor32<0xe>(val);
    rc = rc | loopCompareRor32<0xf>(val);
    return rc;
}

int main() {
    int rc = 0;
    for(u32 val = 0; val <= 0xFFFF; ++val) {
        rc = rc | loopCompareRor32(val);
    }
    return rc;
}