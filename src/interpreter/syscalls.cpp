#include "interpreter/syscalls.h"
#include "interpreter/vm.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "utils/host.h"
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
                Ptr8 buf{arg1};
                size_t count = (size_t)arg2;
                ssize_t nbytes = read(fd, buf, count);
                vm_->set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x1: { // write
                i32 fd = (i32)arg0;
                Ptr8 buf{arg1};
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
                Ptr8 statbufptr{arg1};
                int ret = fstat(fd, statbufptr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x9: { // mmap
                Ptr addr{arg0};
                size_t length = (size_t)arg1;
                int prot = (int)arg2;
                int flags = (int)arg3;
                int fd = (int)arg4;
                off_t offset = (off_t)arg5;
                Ptr ptr = mmap(addr, length, prot, flags, fd, offset);
                vm_->set(R64::RAX, ptr.address());
                return;
            }
            case 0xa: { // mprotect
                Ptr addr{arg0};
                size_t length = (size_t)arg1;
                int prot = (int)arg2;
                int ret = mprotect(addr, length, prot);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xb: { // munmap
                Ptr addr{arg0};
                size_t length = (size_t)arg1;
                int ret = munmap(addr, length);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xc: { // brk
                Ptr addr{arg0};
                Ptr newBreak = brk(addr);
                vm_->set(R64::RAX, newBreak.address());
                return;
            }
            case 0x14: { // writev
                int fd = (int)arg0;
                Ptr iov{arg1};
                int iovcnt = (int)arg2;
                ssize_t nbytes = writev(fd, iov, iovcnt);
                vm_->set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x15: { // access
                Ptr pathname{arg0};
                int mode = (int)arg1;
                int ret = access(pathname, mode);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x3f: { // uname
                Ptr buf{arg0};
                int ret = uname(buf);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x4f: { // getcwd
                Ptr buf{arg0};
                size_t size = (size_t)arg1;
                Ptr ret = getcwd(buf, size);
                vm_->set(R64::RAX, ret.address());
                return;
            }
            case 0x59: { // readlink
                Ptr path {arg0};
                Ptr buf {arg1};
                size_t bufsize = (size_t)arg2;
                ssize_t nchars = readlink(path, buf, bufsize);
                vm_->set(R64::RAX, (u64)nchars);
                return;
            }
            case 0x9e: { // arch_prctl
                i32 code = (i32)arg0;
                Ptr addr{arg1};
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
                Ptr pathname{arg1};
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
        auto data = Host::read(Host::FD{fd}, count);
        if(!data) return (ssize_t)(-1);
        mmu_->copyToMmu(buf, data->data(), data->size());
        return (ssize_t)data->size();
    }

    ssize_t Sys::write(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        return Host::write(Host::FD{fd}, buffer.data(), buffer.size());
    }

    int Sys::close(int fd) {
        return Host::close(Host::FD{fd});
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        std::optional<std::vector<u8>> buf = Host::fstat(Host::FD{fd});
        if(!buf) return -1;
        mmu_->copyToMmu(statbuf, buf->data(), buf->size());
        return 0;
    }

    Ptr Sys::mmap(Ptr addr, size_t length, int prot, int flags, int fd, off_t offset) {
        MAP f = MAP::PRIVATE;
        if(flags & MAP_ANONYMOUS) f = f | MAP::ANONYMOUS;
        if(flags & MAP_FIXED) f = f | MAP::FIXED;
        verify(addr.segment() != Segment::FS);
        u64 base = mmu_->mmap(addr.address(), length, (PROT)prot, f, fd, (int)offset);
        return Ptr{addr.segment(), base};
    }

    int Sys::mprotect(Ptr addr, size_t length, int prot) {
        return mmu_->mprotect(addr.address(), length, (PROT)prot);
    }

    int Sys::munmap(Ptr addr, size_t length) {
        return mmu_->munmap(addr.address(), length);
    }

    Ptr Sys::brk(Ptr addr) {
        verify(addr.segment() != Segment::FS);
        u64 newBrk = mmu_->brk(addr.address());
        return Ptr{addr.segment(), newBrk};
    }

    ssize_t Sys::writev(int fd, Ptr iov, int iovcnt) {
        std::vector<iovec> iovs = mmu_->readFromMmu<iovec>(iov, (size_t)iovcnt);

        std::vector<u8> buffer;
        ssize_t nbytes = 0;
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            void* base = iovs[i].iov_base;
            size_t len = iovs[i].iov_len;
            std::vector<u8> buffer = mmu_->readFromMmu<u8>(Ptr8{(u64)base}, len);
            nbytes += Host::write(Host::FD{fd}, buffer.data(), len);
        }
        return nbytes;
    }

    int Sys::access(Ptr pathname, int mode) {
        std::string path = mmu_->readString(pathname);
        int ret = Host::access(path, mode);
        return ret;
    }

    int Sys::uname(Ptr buf) {
        std::optional<std::vector<u8>> buffer = Host::uname();
        if(!buffer) return -1;
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return 0;
    }

    Ptr Sys::getcwd(Ptr buf, size_t size) {
        std::optional<std::vector<u8>> buffer = Host::getcwd(size);
        if(!buffer) return Ptr{0x0};
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return buf;
    }

    ssize_t Sys::readlink(Ptr pathname, Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto buffer = Host::readlink(path, bufsiz);
        if(!buffer) return -1;
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return 0;
    }

    void Sys::exit_group(int status) {
        (void)status;
        vm_->stop();
    }

    int Sys::arch_prctl(int code, Ptr addr) {
        if(code != ARCH_SET_FS) return -1;
        mmu_->setFsBase(addr.address());
        return 0;
    }

    int Sys::openat(int dirfd, Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_->readString(pathname);
        Host::FD fd = Host::openat(Host::FD{dirfd}, path, flags, mode);
        return fd.fd;
    }

}