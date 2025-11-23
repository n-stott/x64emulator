#include <dlfcn.h>
#include <stdio.h>

int main() {
    // int flags = 0x80000102;
    // void* ptr = dlopen("libshine.so.3", flags);
    // void* ptr = dlopen("libshine.so.3", RTLD_GLOBAL | RTLD_NOW);
    void* ptr = dlopen("/usr/lib/x86_64-linux-gnu/gegl-0.4/ff-save.so", RTLD_GLOBAL | RTLD_NOW);
    printf("dlopen() = %p\n", ptr);
}