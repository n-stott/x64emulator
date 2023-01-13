#include "softfloat.h"

void testA() {
    float32_t x = i32_to_f32(1);
    float32_t y = i32_to_f32(1);
    float32_t z = f32_add(x, y);
}

int main() {
    testA();
}