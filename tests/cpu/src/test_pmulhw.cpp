#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "host/host.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

void testA() {
    u128 dst {
        0x1c001c001c001c,
        0x1a001a001a001a,
    };
    u128 src {
        0xc5a2c5a2c5a2c5a2,
        0xc5a2c5a2c5a2c5a2
    };
    (void)x64::CheckedCpuImpl::pmulhw(dst, src);
}

int main() {
    testA();
    return 0;
}