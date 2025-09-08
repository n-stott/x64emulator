#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testA() {
    u128 dst {0xabcdef0123456789, 0x123456789abcdef0};
    u128 src {0xa2b3c4d5e6f70819, 0x19203a4b5c6d7e8f};
    for(u8 imm = 0; imm < 30; ++imm) {
        (void)x64::CheckedCpuImpl::palignr128(dst, src, imm);
    }
}

void testB() {
    u128 dst {0x2d4f53492e45445f, 0x6700312d39353838};
    u128 src {0x6700312d39353838, 0x6564006e616d7265};
    for(u8 imm = 0; imm < 30; ++imm) {
        (void)x64::CheckedCpuImpl::palignr128(dst, src, imm);
    }
}

int main() {
    testA();
    testB();
    return 0;
}