#include <stdlib.h>
#include <cstdio>

void test1() {
    void* ptr = malloc(2);
    if(!!ptr) {
        puts("malloc(2) != nullptr");
    } else {
        puts("malloc(2) == nullptr");
    }
    free(ptr);
}

struct VoidPtrList {
    void* elem = nullptr;
    VoidPtrList* next = nullptr;
};

void test2() {
    VoidPtrList allocations;
    VoidPtrList* current = &allocations;
    for(int i = 0; i < 0x100; ++i) {
        current->elem = malloc(0x100000);
        if(!current->elem) {
            puts("Failed to allocate 0x10000 bytes");
            break;
        }
        current->next = (VoidPtrList*)malloc(sizeof(VoidPtrList));
        if(!current->next) {
            puts("Failed to allocate VoidPtrList");
            break;
        }
        current->next->elem = nullptr;
        current->next->next = nullptr;
        current = current->next;
    }
    current = &allocations;
    while(!!current) {
        VoidPtrList* next = current->next;
        free(current->elem);
        if(current != &allocations) free(current);
        current = next;
    }
}

int main() {
    test1();
    test2();
}