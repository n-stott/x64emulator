#include <stdlib.h>
#include <cstdio>

void test1() {
    void* ptr = malloc(2);
    if(!!ptr) {
        puts("malloc(2) != nullptr");
    } else {
        puts("malloc(2) == nullptr");
    }
}

int main() {
    test1();
}