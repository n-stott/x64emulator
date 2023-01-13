#include <algorithm>
#include <climits>
#include <cstdio>
#include <string>

long long testMaxA() {
    long long a = 10;
    long long b = 5;
    long long c = std::max(a, b);
    std::string s = "max(10, 5) = " + std::to_string(c);
    std::puts(s.c_str());
    return c;
}

long long testMaxB() {
    long long a = 5;
    long long b = 10;
    long long c = std::max(a, b);
    std::string s = "max(5, 10) = " + std::to_string(c);
    std::puts(s.c_str());
    return c;
}

bool testOverflow() {
    long long a = 10ll;
    long long b = 20ll;
    long long LL_MAX = (long long)(LONG_LONG_MAX);
    bool c = (LL_MAX - a <= b);
    if(c) {
        std::puts("10 + 20 overflows");
    } else {
        std::puts("10 + 20 does not overflow");
    }
    return c;
}

void testLoop() {
    long long maxSize = -1;
    for (long long val = 0; val <= maxSize; val++) {
        std::puts("A");
        break;
    }
}

int main() {
    testMaxA();
    testMaxB();
    testOverflow();
    testLoop();
}