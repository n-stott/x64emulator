#include <cassert>
#include <cstdint>

void testScasb() {
    const char* s = "A string of characters";
    uint64_t counter = -1;
    asm volatile(
        "xor %%eax, %%eax\n"
        "mov %1, %%rdi\n"
        "mov %0, %%rcx\n"
        "repnz scasb\n"
        "mov %%rcx, %0\n"
        : "+r"(counter)
        : "r"(s)
        : "rax", "rdi", "rcx"
    );
    assert(counter == 0xffffffffffffffe8);
}

void testScasw() {
    const char* s = "AABBCCDDEE\0\0";
    uint64_t counter = -1;
    asm volatile(
        "xor %%eax, %%eax\n"
        "mov %1, %%rdi\n"
        "mov %0, %%rcx\n"
        "repnz scasw\n"
        "mov %%rcx, %0\n"
        : "+r"(counter)
        : "r"(s)
        : "rax", "rdi", "rcx"
    );
    assert(counter == 0xfffffffffffffff9);
}

void testScasd() {
    const char* s = "AABBCCDDEEFF\0\0\0\0";
    uint64_t counter = -1;
    asm volatile(
        "xor %%eax, %%eax\n"
        "mov %1, %%rdi\n"
        "mov %0, %%rcx\n"
        "repnz scasl\n"
        "mov %%rcx, %0\n"
        : "+r"(counter)
        : "r"(s)
        : "rax", "rdi", "rcx"
    );
    assert(counter == 0xfffffffffffffffb);
}

int main() {
    testScasb();
    testScasw();
    testScasd();
}