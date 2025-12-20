#include "host/hostmemory.h"
#include "verify.h"
#include <stdexcept>
#include <stdio.h>

#ifdef MSVC_COMPILER
#include "Windows.h"
#include "Memoryapi.h"
#else
#include <sys/mman.h>
#endif

namespace host {

    std::optional<VirtualMemoryRange> VirtualMemoryRange::tryCreate(u64 size) {
        u8* base = HostMemory::tryGetVirtualMemoryRange(size);
        if(!base) return {};
        return VirtualMemoryRange(base, size);
    }

    VirtualMemoryRange::~VirtualMemoryRange() {
        if(!base_) return;
        if(!HostMemory::tryReleaseVirtualMemoryRange(base_, size_)) {
            verify(false, "Unable to release virtual memory range");
        }
    }

    VirtualMemoryRange::VirtualMemoryRange(VirtualMemoryRange&& other) {
        base_ = other.base_;
        size_ = other.size_;
        other.base_ = nullptr;
        other.size_ = 0;
    }

    VirtualMemoryRange& VirtualMemoryRange::operator=(VirtualMemoryRange&& other) {
        base_ = other.base_;
        size_ = other.size_;
        other.base_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    u8* HostMemory::tryGetVirtualMemoryRange(u64 size) {
#ifdef MSVC_COMPILER
        u8* ptr = (u8*)VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_NOACCESS);
        return ptr;
#else
        u8* ptr = (u8*)::mmap(nullptr, size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        if(ptr == (u8*)MAP_FAILED) return nullptr;
        return ptr;
#endif
    }

    bool HostMemory::tryReleaseVirtualMemoryRange(u8* base, u64 size) {
#ifdef MSVC_COMPILER
        (void)size;
        return VirtualFree(base, 0, MEM_RELEASE);
#else
        if(::munmap(base, size) < 0) {
            return false;
        }
        return true;
#endif
    }

    bool HostMemory::tryProtectVirtualMemoryRange(u8* base, u64 size, BitFlags<Protection> protection) {
#ifdef MSVC_COMPILER
        u32 prot = [=]() -> u32 {
            bool read = protection.test(Protection::READ);
            bool write = protection.test(Protection::WRITE);
            bool exec = protection.test(Protection::EXEC);
            if (!exec) {
                if (read && !write) {
                    return PAGE_READONLY;
                } else if (write) {
                    return PAGE_READWRITE;
                } else {
                    return PAGE_NOACCESS;
                }
            } else {
                if (read && !write) {
                    return PAGE_EXECUTE_READ;
                }
                else if (write) {
                    return PAGE_EXECUTE_READWRITE;
                }
                else {
                    return PAGE_EXECUTE;
                }
            }
            return 0;
        }();
        DWORD oldProt { 0 };
        return VirtualProtect(base, size, prot, &oldProt);
#else
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
#endif
    }

}