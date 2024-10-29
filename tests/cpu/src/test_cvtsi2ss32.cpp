#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u128 dst {
        0xff1c1c1cff1c1c1c,
        0xff1c1c1cff1c1c1c,
    };
    
    u32 src = 0x4;

    (void)x64::CheckedCpuImpl::cvtsi2ss32(dst, src);
}