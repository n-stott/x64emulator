#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "cputestutils.h"

void testA() {
    u64 dst = 0x69768383;
    u64 src = 0x303030303030303;
    (void)x64::CheckedCpuImpl::pshufb64(dst, src);
}

void testB() {
    u128 dst = {0xff000088ff000088, 0xff000088ff000088};
    u128 src = {0x8080800680808002, 0x8080800e8080800a};
    (void)x64::CheckedCpuImpl::pshufb128(dst, src);
}

int main() {
    testA();
    testB();
    return 0;
}