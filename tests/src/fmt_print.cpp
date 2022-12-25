#include <fmt/core.h>

void test1() {
    fmt::print("abc\n");
}

void test2() {
    fmt::print("{}\n", "abc");
}

void test3() {
    fmt::print("{}\n", 1);
}

int main() {
    test1();
    test2();
    test3();
}