#include <string>

void test0() {
    std::string s;
}

void test1() {
    std::string s = "abc";
}

void test2() {
    std::string s("abc");
}

void test3() {
    std::string s = "abc";
    s += "def";
}

int main() {
    test0();
    test1();
    test2();
    test3();
}