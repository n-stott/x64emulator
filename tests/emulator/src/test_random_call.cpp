#include <array>
#include <cstdint>

__attribute__((noinline)) void f0() { }
__attribute__((noinline)) void f1() { }
__attribute__((noinline)) void f2() { }
__attribute__((noinline)) void f3() { }
__attribute__((noinline)) void f4() { }
__attribute__((noinline)) void f5() { }
__attribute__((noinline)) void f6() { }
__attribute__((noinline)) void f7() { }

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