#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

int main() {
    {
        u64 src1 = 0x7b08ded0d1c62516;
        u64 src2 = 0xf6e7f5e78019babf;
        
        x64::Flags flags;
        (void)x64::CheckedCpuImpl::mul64(src1, src2, &flags);
    }

    {
        u64 src1 = 0x28;
        u64 src2 = 0xc000000000000000;
        
        x64::Flags flags;
        (void)x64::CheckedCpuImpl::mul64(src1, src2, &flags);
    }
    
    return 0;
}