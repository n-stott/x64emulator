#include "interpreter/syscalls.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <sys/stat.h>

namespace x64 {

    int Sys::fstat(int fd, Ptr8 statbuf) {
        struct stat st;
        int rc = ::fstat(fd, &st);
        u8 buf[sizeof(st)];
        memcpy(&buf, &st, sizeof(st));
        mmu_->copyToMmu(statbuf, buf, sizeof(buf));
        return rc;
    }

    u64 Sys::mmap(u64 addr, size_t length, int prot, int flags, int fd, off_t offset) {
        fmt::print("mmap(addr={:#x}, length={:#x}, prot={:#x}, flags={:#x}, fd={}, offset={:#x})\n", addr, length, prot, flags, fd, offset);
        return mmu_->mmap(addr, length, (PROT)prot, flags, fd, (int)offset);
    }

    int Sys::munmap(u64 addr, size_t length) {
        fmt::print("munmap(addr={:#x}, length={:#x})\n", addr, length);
        return mmu_->munmap(addr, length);
    }

    void Sys::exit_group(int status) {
        (void)status;
        interpreter_->stop();
    }

}