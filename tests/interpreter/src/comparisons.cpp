#include <limits>

int testCmp() {
    int x[4];
    x[0] = 10;
    x[1] = -4;
    x[2] = -7;
    x[3] = -17;

    int ma = -std::numeric_limits<int>::max();
    for(int i = 0; i < 4; ++i) {
        int v = x[i];
        if(v > ma) {
            ma = v;
        }
    }
    return ma;
}

int main() {
    testCmp();
}