#include "x64/checkedcpuimpl.h"
#include "utils.h"
#include "cputestutils.h"

void testA() {
    u128 a {
        0xc041c606feeaaaa6,
        0x3fe462f380000000
    };
    u128 b {
        0xffffffffffffffff,
        0xffffffffffffffff
    };
    (void)x64::CheckedCpuImpl::minps(a, b, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::minps(b, a, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::maxps(a, b, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::maxps(b, a, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::minpd(a, b, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::minpd(b, a, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::maxpd(a, b, x64::SIMD_ROUNDING::NEAREST);
    (void)x64::CheckedCpuImpl::maxpd(b, a, x64::SIMD_ROUNDING::NEAREST);
}

int main() {
    testA();
    return 0;
}