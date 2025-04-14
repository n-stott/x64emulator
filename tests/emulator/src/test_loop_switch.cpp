#include <numeric>
#include <cstdio>
#include <cstring>

int main() {
    volatile long long counter = 0;
    long long counters[16];
    memset(counters, 0, sizeof(counters));
#ifndef NDEBUG
    while(counter != 10'000) {
#else
    while(counter != 10'000'000) {
#endif
        switch(counter & 0xf) {
            case 0: ++counters[4]; break;
            case 1: ++counters[5]; break;
            case 2: ++counters[6]; break;
            case 3: ++counters[7]; break;
            case 4: ++counters[0]; break;
            case 5: ++counters[1]; break;
            case 6: ++counters[2]; break;
            case 7: ++counters[3]; break;
            case 8: ++counters[8]; break;
            case 9: ++counters[9]; break;
            case 10: ++counters[10]; break;
            case 11: ++counters[11]; break;
            case 12: ++counters[12]; break;
            case 13: ++counters[13]; break;
            case 14: ++counters[14]; break;
            case 15: ++counters[15]; break;
        }
        ++counter;
    }
    printf("counters[4]   = %lld\n", counters[4]);
    printf("counters[5]   = %lld\n", counters[5]);
    printf("counters[6]   = %lld\n", counters[6]);
    printf("counters[7]   = %lld\n", counters[7]);
    printf("counters[0]   = %lld\n", counters[0]);
    printf("counters[1]   = %lld\n", counters[1]);
    printf("counters[2]   = %lld\n", counters[2]);
    printf("counters[3]   = %lld\n", counters[3]);
    printf("counters[8]   = %lld\n", counters[8]);
    printf("counters[9]   = %lld\n", counters[9]);
    printf("counters[10]  = %lld\n", counters[10]);
    printf("counters[11]  = %lld\n", counters[11]);
    printf("counters[12]  = %lld\n", counters[12]);
    printf("counters[13]  = %lld\n", counters[13]);
    printf("counters[14]  = %lld\n", counters[14]);
    printf("counters[15]  = %lld\n", counters[15]);
    long long sum = std::accumulate(counters, counters+16, 0);
    if(sum != 0) {
        return 0;
    } else {
        return 1;
    }
}