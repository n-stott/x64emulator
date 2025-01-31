#include <sys/mman.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

std::vector<uint8_t> instructions { 0x48, 0x85, 0xFF, 0x74, 0x02, 0x75, 0x08, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x57, 0x48, 0xFF, 0xCF, 0xE8, 0xE8, 0xFF, 0xFF, 0xFF, 0x5F, 0x48, 0xF7, 0xE7, 0xC3 };

int main() {
    void* ptr = ::mmap(nullptr, 0x1000, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if(ptr == (void*)MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    std::memset(ptr, 0, 0x1000);
    std::memcpy(ptr, instructions.data(), instructions.size());

    using f_ptr = int(*)(int);

    f_ptr f = (f_ptr)ptr;
    int op = 1;
    int value = f(op);

    printf("%d! = %d\n", op, value);
    return 0;
}