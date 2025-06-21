#include <numeric>
#include <cstdio>
#include <cstring>

__attribute__((noinline)) long long inc(long long x) {
    return x+1;
}

int main() {
    volatile long long counter = 0;
    long long sum = 0;
#ifndef NDEBUG
    while(counter != 10'000) {
#else
    while(counter != 10'000'000) {
#endif
        sum = inc(sum);
        ++counter;
    }
    printf("sum = %lld\n", sum);
    return 0;
}