#include "x64/checkedcpuimpl.h"

void testA() {
    u128 dst {
        0x3f80000000000000,
        0x3f80000000000000
    };
    u128 src {
        0x0,
        0x3f8000003f800000
    };
    u8 order = 0x1c;
    (void)x64::CheckedCpuImpl::insertpsReg(dst, src, order);
}

int main() {
    testA();
    return 0;
}