#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u128 src {
        0x489fe555489fe555,
        0x489fb000489fb000,
    };

    (void)x64::CheckedCpuImpl::cvtps2dq(src, x64::SIMD_ROUNDING::NEAREST);
}