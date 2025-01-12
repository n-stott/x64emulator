#include "x64/checkedcpuimpl.h"
#include "utils.h"
#include "cputestutils.h"

void testA() {
    u128 dst {
        0x1234567890abcdef,
        0xfedcba0987654321,
    };
    u128 src {
        0x0,
        0x0
    };
    (void)x64::CheckedCpuImpl::movss(dst, src);
}

int main() {
    testA();
    return 0;
}