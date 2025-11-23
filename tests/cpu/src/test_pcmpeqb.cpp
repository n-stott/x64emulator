#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"
#include <fmt/format.h>

void testA() {
    u128 dst {0x0,                0x0               };
    u128 src {0x696c00332e6f732e, 0x656e69687362696c};
    u128 res = x64::CheckedCpuImpl::pcmpeqb128(dst, src);
    fmt::println("res = {:#x} {:#x}", res.hi, res.lo);
}

int main() {
    testA();
    return 0;
}