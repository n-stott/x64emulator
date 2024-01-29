#include "interpreter/cpuimpl.h"
#include "utils/utils.h"
#include "fmt/core.h"

long double runFrndint(long double x, x64::ROUNDING rounding) {
    {
        unsigned short cw = 0xfedc;
        asm volatile("fnstcw %0" : "=m" (cw));
        cw = (unsigned short)(cw | ((unsigned int)rounding << 10));
        asm volatile("fldcw %0" :: "m" (cw));
    }
    {
        asm volatile("fldt %0" :: "m"(x));
        asm volatile("frndint");
        asm volatile("fstpt %0" : "=m"(x));
    }
    {
        unsigned short cw = 0xfedc;
        asm volatile("fnstcw %0" : "=m" (cw));
        cw = (unsigned short)(cw & ~(0x3 << 10));
        asm volatile("fldcw %0" :: "m" (cw));
    }
    return x;
}

long double runFrndintVirtual(long double x, x64::ROUNDING rounding) {
    x64::X87Fpu fpu;
    fpu.control().rc = rounding;
    f80 r = x64::CpuImpl::frndint(f80::fromLongDouble(x), &fpu);
    return f80::toLongDouble(r);
}

int compareFrndint(long double x, x64::ROUNDING rounding) {
    long double native = runFrndint(x, rounding);
    long double virt = runFrndintVirtual(x, rounding);
    int diff = ::memcmp(&native, &virt, 10);
    if(!diff) return 0;
    fmt::print(stderr, "frndint {} with rounding {} failed\n", x, (int)rounding);
    fmt::print(stderr, "native : {}\n", native);
    fmt::print(stderr, "virtual: {}\n", virt);
    return 1;
}

int main() {
    std::vector<long double> testCases {{
        0.0, -0.0, 1.0, 1.5, 0.5, -0.5, 123.1, 123.0, -12345, 
        std::numeric_limits<long double>::infinity(), -std::numeric_limits<long double>::infinity(),
        std::numeric_limits<long double>::max(), -std::numeric_limits<long double>::max(),
        +2.5, +3.5, +4.5, +5.5, +6.5,
        -2.5, -3.5, -4.5, -5.5, -6.5,
    }};

    int rc = 0;
    for(long double d : testCases) {
        rc |= compareFrndint(d, x64::ROUNDING::NEAREST);
        rc |= compareFrndint(d, x64::ROUNDING::DOWN);
        rc |= compareFrndint(d, x64::ROUNDING::UP);
        rc |= compareFrndint(d, x64::ROUNDING::ZERO);
    }
    return rc;
}