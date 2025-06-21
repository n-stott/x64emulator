#include <stdio.h>
#include <setjmp.h>

jmp_buf my_jump_buffer;

void foo(int status) {
    printf("foo(%d) called\n");
    longjmp(my_jump_buffer, status+1);
}

int main(void) {
    volatile int count = 0;
    if(setjmp(my_jump_buffer) != 5) {
        foo(++count);
    }
}