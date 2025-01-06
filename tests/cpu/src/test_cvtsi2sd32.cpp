#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u32 src = 48;

    u128 dst {
        0x4060000000000000,
        0x0
    };

    (void)x64::CheckedCpuImpl::cvtsi2sd32(dst, src);
}