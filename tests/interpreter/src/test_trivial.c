#include <stdlib.h>
#include <malloc.h>

void f() {
    // __malloc_hook = 0;
    char* ptr = malloc(8);
    free(ptr);
}

int main(int argc, char** argv) {
    if(1) f();
    return argc;
}
