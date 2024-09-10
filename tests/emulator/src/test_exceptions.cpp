#include <exception>
#include <cstdio>

struct Exception : public std::exception {

};

void testA() {
    try {
        throw Exception{};
    } catch(...) {
        std::puts("exception caught");
    }
}

int main() {
    testA();
}