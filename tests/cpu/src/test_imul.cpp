#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "x64/flags.h"
#include "host/host.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <vector>

void testA() {
    u64 src1 = 3;
    u64 src2 = 12297829382473034411ull;

    x64::Flags flags;
    (void)x64::CheckedCpuImpl::imul64(src1, src2, &flags);
}

void testB() {
    u64 src1 = 0;
    u64 src2 = 12297829382473034411ull;

    x64::Flags flags;
    (void)x64::CheckedCpuImpl::imul64(src1, src2, &flags);
}

void testC() {
    u64 src1 = 2;
    u64 src2 = 6148914691236517206ull;

    x64::Flags flags;
    (void)x64::CheckedCpuImpl::imul64(src1, src2, &flags);
}

int main() {
    testA();
    testB();
    testC();
    return 0;
}