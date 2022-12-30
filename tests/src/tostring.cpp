#include <string>
#include <cstdio>

void testA1() {
    std::string s = std::to_string(123);
    std::puts(s.c_str());
}

int main() {
    testA1();
}