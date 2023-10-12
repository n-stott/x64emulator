#include <limits>
#include <cassert>

int testAddFloat() {
    float x = 2.375;
    float y = 4.5;
    float z = x+y;
    if(z != 6.875) return 1;
    return 0;
}

int testAddDouble() {
    double x = 2.375;
    double y = 4.5;
    double z = x+y;
    if(z != 6.875) return 1;
    return 0;
}

void testCmp() {
    assert(0.0 <= 1.0);
    assert(-1.0 <= 0.0);
}

void testInf() {
    double inf = std::numeric_limits<double>::infinity();
    assert(0.0 <= inf);
    assert(1.0 <= inf);
    assert(inf == inf);
    double nan = inf - inf;
    assert(nan != nan);
}

void testNaN() {
    double inf = std::numeric_limits<double>::infinity();
    double nan = inf - inf;
    assert(nan != nan);
    assert(!(0.0 <= nan));
}

int main() {
    testAddFloat();
    testAddDouble();
    testCmp();
    testInf();
    testNaN();
}