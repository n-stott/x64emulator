#include "host/hostmemory.h"
#include <stdexcept>
#include <stdio.h>
#include <sys/mman.h>

namespace host {

    u8* HostMemory::getLowestPossibleVirtualMemoryRange(u64 size) {
        for(u64 base = 0x1000; base < 0x100'000; base += 0x1000) {
            u8* ptr = (u8*)::mmap((void*)base, size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED|MAP_FIXED_NOREPLACE, 0, 0);
            if(ptr == (u8*)MAP_FAILED) continue;
            if(ptr != (void*)base) throw std::logic_error{"Unable to mmap at fixed address"};
            return ptr;
        }
        throw std::logic_error{"Unable to get virtual memory range within [0x1000, 0x100'000]"};
        return nullptr;
    }

    void HostMemory::releaseVirtualMemoryRange(u8* base, u64 size) {
        if(::munmap(base, size) < 0) {
            perror("munmap");
            throw std::logic_error{"Unable to release virtual memory range"};
        }
    }

    void HostMemory::protectVirtualMemoryRange(u8* base, u64 size, BitFlags<Protection> protection) {
        if(base == nullptr) {
            if(protection.any()) throw std::logic_error{"Cannot mprotect nullpage with anything other than NONE"};
            return;
        }
        int prot = PROT_NONE;
        if(protection.test(Protection::READ)) prot |= PROT_READ;
        if(protection.test(Protection::WRITE)) prot |= PROT_WRITE;
        if(::mprotect(base, size, prot) < 0) {
            perror("mprotect");
            throw std::logic_error{"Unable to protect virtual memory range"};
        }
        // printf("mprotect(%p, %lx, %c%c)\n", base, size, protection.test(Protection::READ) ? 'R' : ' ',
        //                                                 protection.test(Protection::WRITE) ? 'W' : ' ');
    }

}