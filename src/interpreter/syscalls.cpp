#include "interpreter/syscalls.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

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

    int Sys::mprotect(u64 addr, size_t length, int prot) {
        fmt::print("mprotect(addr={:#x}, length={:#x}, prot={:#x})\n", addr, length, prot);
        return mmu_->mprotect(addr, length, (PROT)prot);
    }

    int Sys::munmap(u64 addr, size_t length) {
        fmt::print("munmap(addr={:#x}, length={:#x})\n", addr, length);
        return mmu_->munmap(addr, length);
    }

    int Sys::uname(u64 buf) {
        struct utsname buffer;
        int ret = ::uname(&buffer);
        mmu_->copyToMmu(Ptr8{Segment::DS, buf}, (const u8*)&buffer, sizeof(buffer));
        return ret;
    }

    ssize_t Sys::readlink(u64 pathname, u64 buf, size_t bufsiz) {
        Ptr8 ptr{Segment::DS, pathname};
        while(mmu_->read8(ptr) != 0) ++ptr;

        std::vector<char> path;
        path.resize(ptr.address-pathname, 0x0);
        mmu_->copyFromMmu((u8*)path.data(), Ptr8{Segment::DS, pathname}, path.size());

        std::vector<char> buffer;
        buffer.resize(bufsiz, 0x0);
        ssize_t ret = ::readlink(path.data(), buffer.data(), buffer.size());
        if(ret < 0) return ret;

        mmu_->copyToMmu(Ptr8{Segment::DS, buf}, (const u8*)buffer.data(), ret);
        return ret;
    }

    u64 Sys::brk(u64 addr) {
        return mmu_->brk(addr);
    }

    void Sys::exit_group(int status) {
        (void)status;
        interpreter_->stop();
    }

    int Sys::arch_prctl(int code, u64 addr) {
        if(code != ARCH_SET_FS) return -1;
        mmu_->setFsBase(addr);
        return 0;
    }

}