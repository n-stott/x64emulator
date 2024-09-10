#include <sstream>
#include <locale.h>
#include <cstdio>
#include <iomanip>
#include <iostream>

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

void testD() {
    std::stringstream ss;
    int x;
    ss >> x;
}

int testE() {
    std::stringstream ss;
    ss << 123;
    int x;
    ss >> x;
    return x;
}

void testF() {
    std::cout.width(3);
    std::cout << 1;
}

void testG() {
    std::cout << 100;
}

int main() {
    testA();
    testB();
    testC();
    testD();
    testE();
    testF();
    testG();
}