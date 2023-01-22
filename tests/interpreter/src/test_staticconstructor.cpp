#include <vector>

struct S {
    S() {
        x.resize(100, 123456789);
    }
    std::vector<int> x;
};

static S s;

int test1() {
    return s.x.size();
}

int main() {
    test1();
}