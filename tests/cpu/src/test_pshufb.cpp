#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testA() {
    u64 dst = 0x69768383;
    u64 src = 0x303030303030303;
    (void)x64::CheckedCpuImpl::pshufb64(dst, src);
}

int main() {
    testA();
    return 0;
}