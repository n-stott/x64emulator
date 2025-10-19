#include "x64/checkedcpuimpl.h"

void testA() {
    u128 dst {0x407cc802, 0x0};
    u128 src {0x407cc802, 0x0};
    (void)x64::CheckedCpuImpl::roundss128(dst, src, 0x8, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::roundss128(dst, src, 0x9, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::roundss128(dst, src, 0xa, x64::SIMD_ROUNDING::NEAREST);
}

int main() {
    testA();
    return 0;
}