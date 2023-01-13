#include <vector>

struct S {
    S() {
        x.resize(100, 123456789);
    }
    std::vector<int> x;
};

static S s;

int main() {
    return s.x.size();
}