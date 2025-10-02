#if defined(__GNUC__) && defined(__clang__)
// clang

int main() {
    // ignored
    return 0;
}

#elif defined(__GNUC__) && !defined(__clang__)
// gcc

#include "x64/cpuimpl.h"
#include "x64/types.h"
#include "fmt/core.h"

u32 strlen_core_native(u128 a, u128 b) {
    u128 z { 0, 0 };
    u32 res { 0 };
    asm volatile("pxor %0, %0" : "+x"(z));
    asm volatile("pcmpeqb %1, %0" : "+x"(z) : "x"(a));
    asm volatile("pslldq %1, %0" : "+x"(b) : "i"(0xb));
    asm volatile("pcmpeqb %1, %0" : "+x"(b) : "x"(a));
    asm volatile("psubb %1, %0" : "+x"(z) : "x"(b));
    asm volatile("pmovmskb %1, %0" : "+r"(res) : "x"(z));
    return res;
}

u32 strlen_core_virtual(u128 xmm1, u128 xmm2) {
    using namespace x64;
    u128 xmm0 { 0, 0 };
    xmm0 = CpuImpl::pcmpeqb128(xmm0, xmm1);
    xmm2 = CpuImpl::pslldq(xmm2, 0xb);
    xmm2 = CpuImpl::pcmpeqb128(xmm2, xmm1);
    xmm2 = CpuImpl::psubb128(xmm2, xmm0);
    u32 res = CpuImpl::pmovmskb128(xmm2);
    return res;
}

int main() {
    u128 a { 0x2e6362696c003832, 0x3166627573783436 };
    u128 b { 0x362e6f732e2b, 0x2b6364747362696c };
    u32 res_native = strlen_core_native(a, b);
    u32 res_virtual = strlen_core_virtual(a, b);
    if(res_native != res_virtual) {
        fmt::print("res_native={}\n", res_native);
        fmt::print("res_virtual={}\n", res_virtual);
        return 1;
    }
    return 0;
}



#endif