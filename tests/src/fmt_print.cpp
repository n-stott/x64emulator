#include <fmt/core.h>
#include <cstdio>

void testA1() {
    std::string s = "abc";
    std::puts(s.c_str());
}

void testA2() {
    std::string s = "abc";
    s += "def";
    std::puts(s.c_str());
}

void testA3() {
    std::string s = "a very long string that cannot be sso'ed";
    std::puts(s.c_str());
}

void testB1() {
    std::string s = fmt::format("abc");
    std::puts(s.c_str());
}

void testB2() {
    std::string s = fmt::format("{}", "abc");
    std::puts(s.c_str());
}

void testB3() {
    std::string s = fmt::format("{}", 1);
    std::puts(s.c_str());
}

int main() {
    testA1();
    testA2();
    testA3();
    testB1();
    testB2();
    testB3();
}