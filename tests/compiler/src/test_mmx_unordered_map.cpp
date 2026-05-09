#include <cstdint>
#include <unordered_map>

int main() {
    std::unordered_map<int, int> m;
    for(int i = 0; i < 1'000'000; ++i) {
        int v = i%1'000;
        m[v] = i;

        // Without this block, it runs in ~50ms in release mode
        {
            // With this block, it also runs in ~50ms in release mode
            uint64_t mm0 { 0 };
            asm volatile("movq %%mm0, %0\n"
                        "movq %0, %%mm0\n"
            // If we omit this instruction, it jumps up to 1.6s in release mode
                        "emms\n"
                : "+m"(mm0) : );
        }
    }
}