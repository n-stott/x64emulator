#include <unistd.h>
#include <stdio.h>

int main() {
    char* ptr = (char*)sbrk(0);
    if(brk(ptr + 0x2000) < 0) {
        perror("brk");
        return 1;
    }

    ptr[0x1000] = 0xab;

    if(brk(ptr) < 0) {
        perror("undo brk");
        return 1;
    }

    if(brk(ptr + 0x2000) < 0) {
        perror("brk");
        return 1;
    }

    if(ptr[0x1000] == 0xab) {
        return 1;
    }
    if(ptr[0x1000] != 0x0) {
        return 1;
    }

    return 0;
}