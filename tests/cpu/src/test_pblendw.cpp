#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testA() {
    u128 dst { 0x200020002, 0x100010001 };
    u128 src { 0x2000200020002, 0x1000100010001 };
    for(u16 mask = 0; mask < 0x100; ++mask) {
        (void)x64::CheckedCpuImpl::pblendw(dst, src, (u8)mask);
    }
}

int main() {
    testA();
    return 0;
}