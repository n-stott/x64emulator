#include "utils/utils.h"
#include "interpreter/cpuimpl.h"
#include "fmt/core.h"
#include <emmintrin.h>

using punpck = u128(*)(u128, u128);

static u128 punpcklbwNative(u128 dst, u128 src) {
    __m128i DST;
    __m128i SRC;
    static_assert(sizeof(SRC) == sizeof(src));
    ::memcpy(&DST, &dst, sizeof(dst));
    ::memcpy(&SRC, &src, sizeof(src));
    DST = _mm_unpacklo_epi8(DST, SRC);
    ::memcpy(&dst, &DST, sizeof(dst));
    return dst;
}

static u128 punpcklbwVirtual(u128 dst, u128 src) {
    return x64::Impl::punpcklbw(dst, src);
}

int compare(std::string name, punpck native, punpck virt, u128 a, u128 b) {
    u128 rn = native(a, b);
    u128 rv = virt(a, b);

    if(rn.lo == rv.lo
    && rn.hi == rv.hi) return 0;

    fmt::print(stderr, "{} {:#x}{:#x} {:#x}{:#x} failed\n", name, a.hi, a.lo, b.hi, b.lo);
    fmt::print(stderr, "native : res={:#x}{:#x}\n", rn.hi, rn.lo);
    fmt::print(stderr, "virtual: res={:#x}{:#x}\n", rv.hi, rv.lo);
    return 1;
}

int main() {
    int rc = 0;
    rc = rc | compare("punpcklbw", &punpcklbwNative, &punpcklbwVirtual, 
                        u128{0x12345678, 0x87654321}, u128{0x87654321, 0x12345678});
    return rc;
}