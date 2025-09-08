#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testA() {
    constexpr i16 a = 0xeb;
    constexpr i16 b = 0xf8;
    constexpr i16 c = (i16)(a*b);
    static_assert(c == -7256);
    static_assert(c == (i16)0xe3a8);

    u128 dst {0xebebebebebebebeb, 0xebebebebebebebeb};
    u128 src {0xfffefdfcfbfaf9f8, 0x807060504030201};
    (void)x64::CheckedCpuImpl::pmaddubsw128(dst, src);
}

void testB() {
    u64 dst = 0x8080808080808080;
    u64 src = 0x1010101010101010;
    (void)x64::CheckedCpuImpl::pmaddubsw64(dst, src);
}

void testC() {
    u128 dst {0x8080808080808080, 0};
    u128 src {0x1010101010101010, 0};
    (void)x64::CheckedCpuImpl::pmaddubsw128(dst, src);
}

int main() {
    testA();
    testB();
    testC();
    return 0;
}