#include <sstream>
#include <locale.h>
#include <cstdio>

void testA() {
    std::stringstream ss;
    std::string s = ss.str();
    std::puts(s.c_str());
}

void testB() {
    std::stringstream ss;
    ss << 123;
    std::string s = ss.str();
    std::puts(s.c_str());
}

void testC() {
    std::stringstream ss;
    ss << "abc";
    std::string s = ss.str();
    std::puts(s.c_str());
}

int main() {
    testA();
    testB();
    testC();
}