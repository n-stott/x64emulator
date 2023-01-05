#include <cstdio>

struct S1 {
    S1() { std::puts("static S()"); }
    ~S1() { std::puts("static ~S()"); }
};

struct S2 {
    S2() { std::puts("main S()"); }
    ~S2() { std::puts("main ~S()"); }
};

static S1 s1;

int main() {
    S2 s2;
}