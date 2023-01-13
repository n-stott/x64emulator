#include <array>

template<int C>
struct S {
    char array[C];
};

template<int N, int C>
void buildArray() {
    std::array<S<C>, N> x;
}

template<int N, int C>
void pushArray() {
    std::array<S<C>, N> x;
    x[0] = S<C>{};
}

template<int N, int C>
void testArray() {
    pushArray<N, C>();
}

void testArrays() {
    testArray<1, 1>();
    testArray<2, 1>();
    testArray<3, 1>();
    testArray<4, 1>();
    testArray<5, 1>();
    testArray<6, 1>();
    testArray<7, 1>();
    testArray<8, 1>();
    testArray<9, 1>();
    testArray<10, 1>();
    testArray<11, 1>();
    testArray<12, 1>();
    testArray<13, 1>();
    testArray<14, 1>();
    testArray<15, 1>();
    testArray<16, 1>();
    testArray<1, 2>();
    testArray<2, 2>();
    testArray<3, 2>();
    testArray<4, 2>();
    testArray<5, 2>();
    testArray<6, 2>();
    testArray<7, 2>();
    testArray<8, 2>();
    testArray<9, 2>();
    testArray<10, 2>();
    testArray<11, 2>();
    testArray<12, 2>();
    testArray<13, 2>();
    testArray<14, 2>();
    testArray<15, 2>();
    testArray<16, 2>();
}

int main() {
    testArrays();
}