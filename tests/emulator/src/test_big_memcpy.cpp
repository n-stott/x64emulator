#include <cstring>
#include <stdlib.h>

#ifndef NDEBUG
#define BUFSIZE (16*1024)
#else
#define BUFSIZE (1024*1024)
#endif

int main() {
    for(volatile int i = 0; i < 256; ++i) {
        char* ptr1 = (char*)malloc(BUFSIZE);
        char* ptr2 = (char*)malloc(BUFSIZE);

        memset(ptr1, i, BUFSIZE);
        memcpy(ptr2, ptr1, BUFSIZE);

        free(ptr2);
        free(ptr1);
    }
}