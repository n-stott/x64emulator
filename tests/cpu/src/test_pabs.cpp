#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testb() {
    for(u16 i = 0; i <= 0xFF; ++i) {
        u64 m = i;
        u64 src = (m << 48) | (m << 32) | (m << 16) | m;
        (void)x64::CheckedCpuImpl::pabsb64(src);
    }
}

void testw() {
    for(u32 i = 0; i <= 0xFFFF; ++i) {
        u64 m = i;
        u64 src = (m << 32) | m;
        (void)x64::CheckedCpuImpl::pabsw64(src);
    }
}

int main() {
    testb();
    testw();
    return 0;
}