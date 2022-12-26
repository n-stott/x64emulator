#include <fmt/core.h>
#include <cstdio>

void test0() {
    std::string s = "abc\n";
    std::puts(s.c_str());
}

void test1() {
    std::string s = fmt::format("abc\n");
    std::puts(s.c_str());
}

void test2() {
    std::string s = fmt::format("{}\n", "abc");
    std::puts(s.c_str());
}

void test3() {
    std::string s = fmt::format("{}\n", 1);
    std::puts(s.c_str());
}

int main() {
    test0();
    test1();
    test2();
    test3();
}