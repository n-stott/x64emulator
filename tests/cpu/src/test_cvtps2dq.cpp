#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u128 src1 {
        0x489fe555489fe555,
        0x489fb000489fb000,
    };
    (void)x64::CheckedCpuImpl::cvtps2dq(src1, x64::SIMD_ROUNDING::NEAREST);

    u128 src2 {
        0x4786840847864240,
        0x4786840847864240,
    };
    (void)x64::CheckedCpuImpl::cvtps2dq(src2, x64::SIMD_ROUNDING::NEAREST);
}