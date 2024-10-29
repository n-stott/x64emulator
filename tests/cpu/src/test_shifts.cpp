#include "x64/cpuimpl.h"
#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

template<u8 count>
static u64 runSar64Native(u64 val, u8, x64::Flags* flags) {
    return x64::CheckedCpuImpl::sar64(val, count, flags);
    u64 rflags = 0;
    asm volatile("sar %1, %0" : "+r" (val) : "i"(count));
    asm volatile("pushf");
    asm volatile("pop %0" : "=r" (rflags));
    *flags = fromRflags(rflags);
    return val;
}

static u64 runSar64Virtual(u64 val, u8 count, x64::Flags* flags) {
    return x64::CpuImpl::sar64(val, count, flags);
}

template<u8 count>
int compareSar(u64 val) {
    x64::Flags nativeFlags;
    u64 nativeRes = runSar64Native<count>(val, count, &nativeFlags);

    x64::Flags virtFlags;
    u64 virtRes = runSar64Virtual(val, count, &virtFlags);

    if(count == 0) {
        if(virtRes == nativeRes) return 0;
    } else  if(count == 1) {
        if(virtRes == nativeRes
        && virtFlags.carry == nativeFlags.carry
        && virtFlags.overflow == nativeFlags.overflow) return 0;    
    } else {
        if(virtRes == nativeRes
        && virtFlags.carry == nativeFlags.carry) return 0;
    }

    fmt::print(stderr, "op {:#x} {:#x} failed\n", val, count);
    fmt::print(stderr, "native : res={:#x} carry={} overflow={}\n",
                        nativeRes, nativeFlags.carry, nativeFlags.overflow);
    fmt::print(stderr, "virtual: res={:#x} carry={} overflow={}\n",
                        virtRes, virtFlags.carry, virtFlags.overflow);
    return 1;
}

int main() {
    int ret = 0;
    ret |= compareSar<224>(4294967296);
    ret |= compareSar<192>(0);
    x64::Flags flags;
    (void)x64::CheckedCpuImpl::shl64(6917529027641081844ull, 1, &flags);
    return ret;
}