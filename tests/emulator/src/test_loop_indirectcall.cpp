#include <numeric>
#include <cstdio>
#include <cstring>
#include <sched.h>

struct Base {
    virtual long long inc(long long) = 0;    
};

struct Derived1 : public Base {
    __attribute__((noinline)) long long inc(long long x) override {
        return x+1;
    }
};

struct Derived2 : public Base {
    __attribute__((noinline)) long long inc(long long x) override {
        return x+2;
    }
};

int main() {
    volatile long long counter = 0;
    long long sum = 0;
    Derived1 d1;
    Base* bs = &d1;
#ifndef NDEBUG
    while(counter != 10'000) {
#else
    while(counter != 10'000'000) {
#endif
        sum = bs->inc(sum);
        ++counter;
    }
    printf("sum = %lld\n", sum);
    return 0;
}