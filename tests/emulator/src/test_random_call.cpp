#include <array>
#include <cstdint>

#ifdef MSVC_COMPILER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

NOINLINE void f0() {}
NOINLINE void f1() {}
NOINLINE void f2() {}
NOINLINE void f3() {}
NOINLINE void f4() {}
NOINLINE void f5() {}
NOINLINE void f6() {}
NOINLINE void f7() { }

int main() {
    using fptr = void(*)();
    std::array<fptr, 8> fs {{
        f0, f1, f2, f3, f4, f5, f6, f7
    }};

    uint32_t seed = 1;
    for(int i = 0; i < 1'000'000; ++i) {
        seed = ((seed + 5) << 3) ^ ((seed >> 2) - seed - 5) + 0x1337;
        auto f = fs[seed & 7];
        f();
    }
}