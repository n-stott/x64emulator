#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u128 dst {
        0x3f800000bf800000,
        0x3f800000bf800000,
    };

    u128 src {
        0xbf800000bf800000,
        0x3f8000003f800000,
    };

    (void)x64::CheckedCpuImpl::unpcklps(dst, src);
}