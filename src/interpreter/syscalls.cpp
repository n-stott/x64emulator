#include "interpreter/syscalls.h"
#include "interpreter/vm.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <asm/prctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>

namespace x64 {

    void Sys::syscall() {
        u64 sysNumber = vm_->get(R64::RAX);
        u64 arg0 = vm_->get(R64::RDI);
        u64 arg1 = vm_->get(R64::RSI);
        u64 arg2 = vm_->get(R64::RDX);
        u64 arg3 = vm_->get(R64::R10);
        u64 arg4 = vm_->get(R64::R8);
        u64 arg5 = vm_->get(R64::R9);

        switch(sysNumber) {
            case 0x0: { // read
                i32 fd = (i32)arg0;
                Ptr8 buf{Segment::DS, arg1};
                size_t count = (size_t)arg2;
                ssize_t nbytes = read(fd, buf, count);
                vm_->set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x1: { // write
                i32 fd = (i32)arg0;
                Ptr8 buf{Segment::DS, arg1};
                size_t count = (size_t)arg2;
                ssize_t nbytes = write(fd, buf, count);
                vm_->set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x3: { // close
                i32 fd = (i32)arg0;
                int ret = close(fd);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x5: { // fstat
                i32 fd = (i32)arg0;
                Ptr8 statbufptr{Segment::DS, arg1};
                int ret = fstat(fd, statbufptr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x9: { // mmap
                u64 addr = arg0;
                size_t length = (size_t)arg1;
                int prot = (int)arg2;
                int flags = (int)arg3;
                int fd = (int)arg4;
                off_t offset = (off_t)arg5;
                u64 ptr = mmap(addr, length, prot, flags, fd, offset);
                vm_->set(R64::RAX, ptr);
                return;
            }
            case 0xa: { // mprotect
                u64 addr = arg0;
                size_t length = (size_t)arg1;
                int prot = (int)arg2;
                int ret = mprotect(addr, length, prot);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xb: { // munmap
                u64 addr = arg0;
                size_t length = (size_t)arg1;
                int ret = munmap(addr, length);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xc: { // brk
                u64 addr = arg0;
                u64 newBreak = brk(addr);
                vm_->set(R64::RAX, newBreak);
                return;
            }
            case 0x14: { // writev
                int fd = (int)arg0;
                u64 iov = arg1;
                int iovcnt = (int)arg2;
                ssize_t nbytes = writev(fd, iov, iovcnt);
                vm_->set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x15: { // access
                u64 pathname = arg0;
                int mode = (int)arg1;
                int ret = access(pathname, mode);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x3f: { // uname
                u64 buf = arg0;
                int ret = uname(buf);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x4f: { // getcwd
                u64 buf = arg0;
                size_t size = (size_t)arg1;
                u64 ret = getcwd(buf, size);
                vm_->set(R64::RAX, ret);
                return;
            }
            case 0x59: { // readlink
                u64 path = arg0;
                u64 buf = arg1;
                size_t bufsize = (size_t)arg2;
                ssize_t nchars = readlink(path, buf, bufsize);
                vm_->set(R64::RAX, (u64)nchars);
                return;
            }
            case 0x9e: { // arch_prctl
                i32 code = (i32)arg0;
                u64 addr = arg1;
                int ret = arch_prctl(code, addr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xe7: { // exit_group
                i32 errorCode = (i32)arg0;
                exit_group(errorCode);
                return;
            }
            case 0x101: { // openat
                int dirfd = (int)arg0;
                u64 pathname = arg1;
                int flags = (int)arg2;
                mode_t mode = (mode_t)arg3;
                int ret = openat(dirfd, pathname, flags, mode);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }

            default: break;
        }
        verify(false, [&]() {
            fmt::print("Syscall {:#x} not handled\n", sysNumber);
        });
    }

    ssize_t Sys::read(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(fd, buffer.data(), count);
        if(nbytes > 0) {
            mmu_->copyToMmu(buf, buffer.data(), (size_t)nbytes);
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
        if(rc < 0) return rc;
        u8 buf[sizeof(st)];
        memcpy(&buf, &st, sizeof(st));
        mmu_->copyToMmu(statbuf, buf, sizeof(buf));
        return rc;
    }

    u64 Sys::mmap(u64 addr, size_t length, int prot, int flags, int fd, off_t offset) {
        MAP f = MAP::PRIVATE;
        if(flags & MAP_ANONYMOUS) f = f | MAP::ANONYMOUS;
        if(flags & MAP_FIXED) f = f | MAP::FIXED;
        return mmu_->mmap(addr, length, (PROT)prot, f, fd, (int)offset);
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

    ssize_t Sys::writev(int fd, u64 iov, int iovcnt) {
        std::vector<struct iovec> iovs;
        iovs.resize((size_t)iovcnt);
        mmu_->copyFromMmu((u8*)iovs.data(), Ptr8{Segment::DS, iov}, (size_t)iovcnt*sizeof(struct iovec));

        std::vector<u8> buffer;
        ssize_t nbytes = 0;
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            void* base = iovs[i].iov_base;
            size_t len = iovs[i].iov_len;
            buffer.resize(len);
            mmu_->copyFromMmu(buffer.data(), Ptr8{Segment::DS, (u64)base}, len);
            nbytes += ::write(fd, buffer.data(), len);
        }
        return nbytes;
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

        mmu_->copyToMmu(Ptr8{Segment::DS, buf}, (const u8*)buffer.data(), (size_t)ret);
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