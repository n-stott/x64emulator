#include "interpreter/syscalls.h"
#include "interpreter/vm.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>

namespace x64 {

    ssize_t Sys::read(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(fd, buffer.data(), count);
        if(nbytes > 0) {
            mmu_->copyToMmu(buf, buffer.data(), nbytes);
        }
        return nbytes;
    }

    ssize_t Sys::write(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        mmu_->copyFromMmu(buffer.data(), buf, count);
        ssize_t nbytes = ::write(fd, buffer.data(), count);
        return nbytes;
    }

    int Sys::close(int fd) {
        return ::close(fd);
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        struct stat st;
        int rc = ::fstat(fd, &st);
        if(rc < 0) return (u64)rc;
        u8 buf[sizeof(st)];
        memcpy(&buf, &st, sizeof(st));
        mmu_->copyToMmu(statbuf, buf, sizeof(buf));
        return (u64)rc;
    }

    u64 Sys::mmap(u64 addr, size_t length, int prot, int flags, int fd, off_t offset) {
        return mmu_->mmap(addr, length, (PROT)prot, flags, fd, (int)offset);
    }

    int Sys::mprotect(u64 addr, size_t length, int prot) {
        return mmu_->mprotect(addr, length, (PROT)prot);
    }

    int Sys::munmap(u64 addr, size_t length) {
        return mmu_->munmap(addr, length);
    }

    u64 Sys::brk(u64 addr) {
        return mmu_->brk(addr);
    }

    int Sys::access(u64 pathname, int mode) {
        Ptr8 ptr{Segment::DS, pathname};
        while(mmu_->read8(ptr) != 0) ++ptr;

        std::vector<char> path;
        path.resize(ptr.address-pathname, 0x0);
        mmu_->copyFromMmu((u8*)path.data(), Ptr8{Segment::DS, pathname}, path.size());

        int ret = ::access(path.data(), mode);
        return ret;
    }

    int Sys::uname(u64 buf) {
        struct utsname buffer;
        int ret = ::uname(&buffer);
        mmu_->copyToMmu(Ptr8{Segment::DS, buf}, (const u8*)&buffer, sizeof(buffer));
        return ret;
    }

    u64 Sys::getcwd(u64 buf, size_t size) {
        std::vector<char> path;
        path.resize(size, 0x0);
        char* cwd = ::getcwd(path.data(), size);
        verify(cwd == path.data());

        mmu_->copyToMmu(Ptr8{Segment::DS, buf}, (const u8*)path.data(), size);
        return buf;
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

    void Sys::exit_group(int status) {
        (void)status;
        vm_->stop();
    }

    int Sys::arch_prctl(int code, u64 addr) {
        if(code != ARCH_SET_FS) return -1;
        mmu_->setFsBase(addr);
        return 0;
    }

    int Sys::openat(int dirfd, u64 pathname, int flags, mode_t mode) {
        std::vector<char> pathnamestring;
        Ptr8 ptr { Segment::DS, pathname };
        while(true) {
            u8 c = mmu_->read8(ptr);
            pathnamestring.push_back((char)c);
            if(c == 0) break;
            ++ptr;
        }
        verify((flags & O_ACCMODE) == O_RDONLY, "Writing to files is not supported");
        return ::openat(dirfd, pathnamestring.data(), flags, mode);
    }

}