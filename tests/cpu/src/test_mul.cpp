#include "utils/utils.h"
#include "interpreter/cpuimpl.h"
#include "interpreter/checkedcpuimpl.h"
#include "interpreter/flags.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    u64 src1 = 0x7b08ded0d1c62516;
    u64 src2 = 0xf6e7f5e78019babf;
    
    x64::Flags flags;
    (void)x64::CheckedCpuImpl::mul64(src1, src2, &flags);
    return 0;
}