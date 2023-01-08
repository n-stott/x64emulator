#include <sstream>
#include <locale.h>

void testA() {
    std::stringstream ss;
}

void testB() {
    std::stringstream ss;
    ss << 123;
}

void testC() {
    std::stringstream ss;
    ss << "abc";
}

int main() {
    testA();
    testB();
    testC();
}