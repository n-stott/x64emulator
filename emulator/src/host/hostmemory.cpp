#include "host/hostmemory.h"
#include "verify.h"
#include <stdexcept>
#include <stdio.h>
#include <sys/mman.h>

namespace host {

    u8* HostMemory::tryGetVirtualMemoryRange(u64 size) {
        u8* ptr = (u8*)::mmap(nullptr, size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(ptr == (u8*)MAP_FAILED) return nullptr;
        return ptr;
    }

    bool HostMemory::tryReleaseVirtualMemoryRange(u8* base, u64 size) {
        if(::munmap(base, size) < 0) {
            return false;
        }
        return true;
    }

    bool HostMemory::tryProtectVirtualMemoryRange(u8* base, u64 size, BitFlags<Protection> protection) {
        if(base == nullptr) {
            verify(protection.any(), "Cannot mprotect nullpage with anything other than NONE");
            return false;
        }
        int prot = PROT_NONE;
        if(protection.test(Protection::READ)) prot |= PROT_READ;
        if(protection.test(Protection::WRITE)) prot |= PROT_WRITE;
        if(protection.test(Protection::EXEC)) prot |= PROT_EXEC;
        if(::mprotect(base, size, prot) < 0) {
            return false;
        }
        return true;
    }

}