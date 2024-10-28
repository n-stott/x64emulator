#include <chrono>
#include <cstdio>
#include <thread>

int main() {
    puts("TIC");
    auto begin = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    auto end = std::chrono::steady_clock::now();
    auto diff = (end-begin).count() / 1'000'000;
    puts("TOC");
    printf("Waited for %ld milliseconds\n", diff);
}