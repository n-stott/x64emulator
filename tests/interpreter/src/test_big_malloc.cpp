#include <vector>

struct A {
    char data[0x1000];
};

int main() {
    std::vector<A> v;
    v.reserve(0x100);
}