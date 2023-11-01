#include "utils/utils.h"
#include "interpreter/cpu.h"
#include "fmt/core.h"
#include <emmintrin.h>

template<u8 order>
u128 runPshufdNative(u128 src) {
    __m128i SRC;
    static_assert(sizeof(SRC) == sizeof(src));
    ::memcpy(&SRC, &src, sizeof(src));
    __m128i DST = _mm_shuffle_epi32(SRC, order);
    u128 dst;
    ::memcpy(&dst, &DST, sizeof(dst));
    return dst;
}

u128 runPshufdVirtual(u128 src, u8 order) {
    return x64::Cpu::execPshufd(src, order);
}

template<u8 order>
int comparePshufd(u128 src) {
    u128 native = runPshufdNative<order>(src);
    u128 virt = runPshufdVirtual(src, order);
    if (native.lo == virt.lo && native.hi == virt.hi) return 0;
    fmt::print(stderr, "pshufd dst, {:#x} {:x}, {:#x} failed\n", src.hi, src.lo, order);
    fmt::print(stderr, "native  = {:#x} {:x}\n", native.hi, native.lo);
    fmt::print(stderr, "virtual = {:#x} {:x}\n", virt.hi, virt.lo);
    return 1;
}

template<int hi>
int loopComparePshufd(u128 src) {
    int rc = 0;
    rc = rc | comparePshufd<(hi << 4) | 0x0>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x1>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x2>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x3>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x4>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x5>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x6>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x7>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x8>(src);
    rc = rc | comparePshufd<(hi << 4) | 0x9>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xa>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xb>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xc>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xd>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xe>(src);
    rc = rc | comparePshufd<(hi << 4) | 0xf>(src);
    return rc;
}

int loopComparePshufd(u128 src) {
    int rc = 0;
    rc = rc | loopComparePshufd<0x0>(src);
    rc = rc | loopComparePshufd<0x1>(src);
    rc = rc | loopComparePshufd<0x2>(src);
    rc = rc | loopComparePshufd<0x3>(src);
    rc = rc | loopComparePshufd<0x4>(src);
    rc = rc | loopComparePshufd<0x5>(src);
    rc = rc | loopComparePshufd<0x6>(src);
    rc = rc | loopComparePshufd<0x7>(src);
    rc = rc | loopComparePshufd<0x8>(src);
    rc = rc | loopComparePshufd<0x9>(src);
    rc = rc | loopComparePshufd<0xa>(src);
    rc = rc | loopComparePshufd<0xb>(src);
    rc = rc | loopComparePshufd<0xc>(src);
    rc = rc | loopComparePshufd<0xd>(src);
    rc = rc | loopComparePshufd<0xe>(src);
    rc = rc | loopComparePshufd<0xf>(src);
    return rc;
}

int main() {
    u128 src { 0x89abcdef, 0x1234567 };
    int rc = loopComparePshufd(src);
    return rc;
}