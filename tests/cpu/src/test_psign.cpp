#include "x64/checkedcpuimpl.h"

void testA() {
    for(u16 i = 0; i < 0x100; ++i) {
        u64 dst = (u64)i;
        for(u16 j = 0; j < 0x100; ++j) {
            u64 src = (u64)j;
            (void)x64::CheckedCpuImpl::psignb64(dst, src);
        }
    }
}

int main() {
    testA();
    return 0;
}