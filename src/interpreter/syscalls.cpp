#include "interpreter/syscalls.h"
#include "interpreter/vm.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "utils/host.h"
#include <numeric>
#include <asm/prctl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
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
            case 0x4: { // stat
                Ptr8 pathname{arg0};
                Ptr8 statbufptr{arg1};
                int ret = stat(pathname, statbufptr);
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
            case 0x8: { // lseek
                int fd = (i32)arg0;
                off_t offset = (off_t)arg1;
                int whence = (int)arg2;
                off_t ret = lseek(fd, offset, whence);
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
            case 0xd: { // rt_sigaction
                int sig = (int)arg0;
                Ptr act(arg1);
                Ptr oact(arg2);
                size_t sigsetsize = (size_t)arg3;
                int ret = rt_sigaction(sig, act, oact, sigsetsize);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xe: { // rt_sigprocmask
                int how = (int)arg0;
                Ptr nset(arg1);
                Ptr oset(arg2);
                size_t sigsetsize = (size_t)arg3;
                int ret = rt_sigprocmask(how, nset, oset, sigsetsize);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x10: { // ioctl
                int fd = (int)arg0;
                unsigned long request = (unsigned long)arg1;
                Ptr argp{arg2};
                int ret = ioctl(fd, request, argp);
                vm_->set(R64::RAX, (u64)ret);
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
            case 0x27: { // getpid
                vm_->set(R64::RAX, (u64)0xface);
                return;
            }
            case 0x3f: { // uname
                Ptr buf{arg0};
                int ret = uname(buf);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x48: { // fcntl
                int fd = (int)arg0;
                int cmd = (int)arg1;
                int arg = (int)arg2;
                int ret = fcntl(fd, cmd, arg);
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
            case 0x63: { //sysinfo
                Ptr info {arg0};
                int ret = sysinfo(info);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x9e: { // arch_prctl
                i32 code = (i32)arg0;
                Ptr addr{arg1};
                int ret = arch_prctl(code, addr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xba: { // gettid
                vm_->set(R64::RAX, (u64)0xfeed);
                return;
            }
            case 0xca: { // futex
                Ptr32 uaddr(arg0);
                int futex_op = (int)arg1;
                uint32_t val = (uint32_t)arg2;
                Ptr timeout(arg3);
                Ptr32 uaddr2(arg4);
                uint32_t val3 = (uint32_t)arg5;
                long ret = futex(uaddr, futex_op, val, timeout, uaddr2, val3);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xda: { // set_tid_address
                Ptr32 tidptr{arg0};
                pid_t ret = set_tid_address(tidptr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xe4: { // clock_gettime
                clockid_t clockid = (clockid_t)arg0;
                Ptr tp(arg1);
                int ret = clock_gettime(clockid, tp);
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
            case 0x111: { // set_robust_list
                Ptr head(arg0);
                size_t len = (size_t)arg1;
                long ret = set_robust_list(head, len);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x112: { // get_robust_list
                pid_t pid = (pid_t)arg0;
                Ptr64 head_ptr(arg1);
                Ptr64 len_ptr(arg2);
                long ret = get_robust_list(pid, head_ptr, len_ptr);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x12e: { // prlimit64
                pid_t pid = (pid_t)arg0;
                int resource = (int)arg1;
                Ptr new_limit(arg2);
                Ptr old_limit(arg3);
                int ret = prlimit64(pid, resource, new_limit, old_limit);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x13e: { // getrandom
                Ptr buf{arg0};
                size_t len = (size_t)arg1;
                int flags = (int)arg2;
                ssize_t ret = getrandom(buf, len, flags);
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
        if(vm_->logSyscalls()) fmt::print("Sys::read(fd={}, buf={:#x}, count={}) = {}\n", fd, buf.address(), count, data ? (int)data->size() : -1);
        if(!data) return (ssize_t)(-1);
        mmu_->copyToMmu(buf, data->data(), data->size());
        return (ssize_t)data->size();
    }

    ssize_t Sys::write(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        ssize_t ret = Host::write(Host::FD{fd}, buffer.data(), buffer.size());
        if(vm_->logSyscalls()) fmt::print("Sys::write(fd={}, buf={:#x}, count={}) = {}\n", fd, buf.address(), count, ret);
        return ret;
    }

    int Sys::close(int fd) {
        int ret = Host::close(Host::FD{fd});
        if(vm_->logSyscalls()) fmt::print("Sys::close(fd={}) = {}\n", fd, ret);
        return ret;
    }

    int Sys::stat(Ptr pathname, Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        std::optional<std::vector<u8>> buf = Host::stat(path);
        if(vm_->logSyscalls()) fmt::print("Sys::stat(path={}, statbuf={:#x}) = {}\n", path, statbuf.address(), buf ? 0 : -1);
        if(!buf) return -1;
        mmu_->copyToMmu(statbuf, buf->data(), buf->size());
        return 0;
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        std::optional<std::vector<u8>> buf = Host::fstat(Host::FD{fd});
        if(vm_->logSyscalls()) fmt::print("Sys::fstat(fd={}, statbuf={:#x}) = {}\n", fd, statbuf.address(), buf ? 0 : -1);
        if(!buf) return -1;
        mmu_->copyToMmu(statbuf, buf->data(), buf->size());
        return 0;
    }

    off_t Sys::lseek(int fd, off_t offset, int whence) {
        if(vm_->logSyscalls()) fmt::print("Sys::lseek(fd={}, offset={:#x}, whence={}) = {}\n", fd, offset, whence);
        return Host::lseek(Host::FD{fd}, offset, whence);
    }

    Ptr Sys::mmap(Ptr addr, size_t length, int prot, int flags, int fd, off_t offset) {
        MAP f = MAP::PRIVATE;
        if(flags & MAP_ANONYMOUS) f = f | MAP::ANONYMOUS;
        if(flags & MAP_FIXED) f = f | MAP::FIXED;
        verify(addr.segment() != Segment::FS);
        u64 base = mmu_->mmap(addr.address(), length, (PROT)prot, f, fd, (int)offset);
        if(vm_->logSyscalls()) fmt::print("Sys::mmap(addr={:#x}, length={}, prot={}, flags={}, fd={}, offset={}) = {:#x}\n",
                                              addr.address(), length, prot, flags, fd, offset, base);
        return Ptr{addr.segment(), base};
    }

    int Sys::mprotect(Ptr addr, size_t length, int prot) {
        int ret = mmu_->mprotect(addr.address(), length, (PROT)prot);
        if(vm_->logSyscalls()) fmt::print("Sys::mprotect(addr={:#x}, length={}, prot={}) = {}\n", addr.address(), length, prot, ret);
        return ret;
    }

    int Sys::munmap(Ptr addr, size_t length) {
        int ret = mmu_->munmap(addr.address(), length);
        if(vm_->logSyscalls()) fmt::print("Sys::munmap(addr={:#x}, length={}) = {}\n", addr.address(), length, ret);
        return ret;
    }

    Ptr Sys::brk(Ptr addr) {
        verify(addr.segment() != Segment::FS);
        u64 newBrk = mmu_->brk(addr.address());
        if(vm_->logSyscalls()) fmt::print("Sys::brk(addr={:#x}) = {:#x}\n", addr.address(), newBrk);
        return Ptr{addr.segment(), newBrk};
    }

    int Sys::rt_sigaction(int sig, Ptr act, Ptr oact, size_t sigsetsize) {
        if(vm_->logSyscalls()) fmt::print("Sys::rt_sigaction({}, {:#x}, {:#x}, {}) = 0\n", sig, act.address(), oact.address(), sigsetsize);
        (void)sig;
        (void)act;
        (void)oact;
        (void)sigsetsize;
        return 0;
    }

    int Sys::rt_sigprocmask(int how, Ptr nset, Ptr oset, size_t sigsetsize) {
        if(vm_->logSyscalls()) fmt::print("Sys::rt_sigprocmask({}, {:#x}, {:#x}, {}) = 0\n", how, nset.address(), oset.address(), sigsetsize);
        (void)how;
        (void)nset;
        (void)oset;
        (void)sigsetsize;
        return 0;
    }

    int Sys::ioctl(int fd, unsigned long request, Ptr argp) {
        if(request == TCGETS) {
            if(vm_->logSyscalls()) fmt::print("Sys::ioctl({}, TCGETS, {:#x})\n", fd, request, argp.address());
            std::optional<std::vector<u8>> buffer = Host::tcgetattr(Host::FD{fd});
            mmu_->copyToMmu(argp, buffer->data(), buffer->size());
            return 0;
        }
        verify(false, [&]() {
            fmt::print("Unsupported ioctl {}\n", request);
        });
        return -1;
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
        if(vm_->logSyscalls()) fmt::print("Sys::writev(fd={}, iov={:#x}, iovcnt={}) = {}\n", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    int Sys::access(Ptr pathname, int mode) {
        std::string path = mmu_->readString(pathname);
        int ret = Host::access(path, mode);
        std::string info;
        if(ret < 0) {
            info = strerror(-ret);
        }
        if(vm_->logSyscalls()) fmt::print("Sys::access(path={}, mode={}) = {} {}\n", path, mode, ret, info);
        return ret;
    }

    int Sys::uname(Ptr buf) {
        std::optional<std::vector<u8>> buffer = Host::uname();
        if(vm_->logSyscalls()) fmt::print("Sys::uname(buf={:#x}) = {}\n", buf.address(), buffer ? 0 : -1);
        if(!buffer) return -1;
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return 0;
    }

    int Sys::fcntl(int fd, int cmd, int arg) {
        if(cmd == F_GETFD) {
            int ret = Host::getfd(Host::FD{fd});
            if(vm_->logSyscalls()) fmt::print("Sys::fcntl(fd={}, cmd={}) = {:#x}\n", fd, "GETFD", ret);
            return ret;
        }
        if(cmd == F_SETFD) {
            int ret = Host::setfd(Host::FD{fd}, arg);
            if(vm_->logSyscalls()) fmt::print("Sys::fcntl(fd={}, cmd={}, arg={}) = {:#x}\n", fd, "SETFD", arg, ret);
            return ret;
        }
        verify(false, [&]() {
            fmt::print("Unsupported fcntl {}\n", cmd);
        });
        return -1;
    }

    Ptr Sys::getcwd(Ptr buf, size_t size) {
        std::optional<std::vector<u8>> buffer = Host::getcwd(size);
        if(vm_->logSyscalls()) fmt::print("Sys::getcwd(buf={:#x}, size={}) = {:#x}\n", buf.address(), size, buffer ? 0 : buf.address());
        if(!buffer) return Ptr{0x0};
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return buf;
    }

    ssize_t Sys::readlink(Ptr pathname, Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto buffer = Host::readlink(path, bufsiz);
        if(vm_->logSyscalls()) fmt::print("Sys::readlink(path={}, buf={:#x}; size={}) = {:#x}\n", path, buf.address(), bufsiz, buffer ? 0 : -1);
        if(!buffer) return -1;
        mmu_->copyToMmu(buf, buffer->data(), buffer->size());
        return 0;
    }

    int Sys::sysinfo(Ptr info) {
        auto buffer = Host::sysinfo();
        if(!buffer) return -1;
        mmu_->copyToMmu(info, buffer->data(), buffer->size());
        return 0;
    }

    void Sys::exit_group(int status) {
        (void)status;
        if(vm_->logSyscalls()) fmt::print("Sys::exit_group(status={})\n", status);
        vm_->stop();
    }

    long Sys::futex(Ptr32 uaddr, int futex_op, uint32_t val, Ptr timeout, Ptr32 uaddr2, uint32_t val3) {
        if(vm_->logSyscalls()) fmt::print("Sys::futex({:#x}, {}, {}, {:#x}, {:#x}, {})\n", uaddr.address(), futex_op, val, timeout.address(), uaddr2.address(), val3);
        return 1;
    }

    pid_t Sys::set_tid_address(Ptr32 ptr) {
        if(vm_->logSyscalls()) fmt::print("Sys::set_tid_address({:#x}) = {}\n", ptr.address(), 1);
        return 1;
    }

    int Sys::clock_gettime(clockid_t clockid, Ptr tp) {
        if(vm_->logSyscalls()) fmt::print("Sys::clock_gettime({}, {:#x})\n", clockid, tp.address());
        return 0;
    }

    int Sys::arch_prctl(int code, Ptr addr) {
        if(vm_->logSyscalls()) fmt::print("Sys::arch_prctl(code={}, addr={:#x}) = {}\n", code, addr.address(), code == ARCH_SET_FS ? 0 : -1);
        if(code != ARCH_SET_FS) return -1;
        mmu_->setFsBase(addr.address());
        return 0;
    }

    int Sys::openat(int dirfd, Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_->readString(pathname);
        Host::FD fd = Host::openat(Host::FD{dirfd}, path, flags, mode);
        if(vm_->logSyscalls()) fmt::print("Sys::openat(dirfd={}, path={}, flags={}, mode={}) = {}\n", dirfd, path, flags, mode, fd.fd);
        return fd.fd;
    }

    long Sys::set_robust_list(Ptr head, size_t len) {
        if(vm_->logSyscalls()) fmt::print("Sys::set_robust_list({:#x}, {}) = 0\n", head.address(), len);
        // maybe we can do nothing ?
        (void)head;
        (void)len;
        return 0;
    }

    long Sys::get_robust_list(int pid, Ptr64 head_ptr, Ptr64 len_ptr) {
        if(vm_->logSyscalls()) fmt::print("Sys::get_robust_list({}, {:#x}, {:#x}) = 0\n", pid, head_ptr.address(), len_ptr.address());
        (void)pid;
        (void)head_ptr;
        (void)len_ptr;
        verify(false, "implement {get,set}_robust_list");
        return 0;
    }

    int Sys::prlimit64(pid_t pid, int resource, Ptr new_limit, Ptr old_limit) {
        if(vm_->logSyscalls()) 
            fmt::print("Sys::prlimit64(pid={}, resource={}, new_limit={:#x}, old_limit={:#x})", pid, resource, new_limit.address(), old_limit.address());
        (void)new_limit;
        if(!old_limit.address()) {
            if(vm_->logSyscalls()) fmt::print(" = 0\n");
            return 0;
        }
        std::vector<u8> oldLimitBuffer;
        oldLimitBuffer.resize(sizeof(struct rlimit), 0x0);
        int ret = Host::prlimit64(pid, resource, nullptr, &oldLimitBuffer);
        if(ret < 0) {
            if(vm_->logSyscalls()) fmt::print(" = {}\n", ret);
            return ret;
        } else {
            struct rlimit oldLimit;
            std::memcpy(&oldLimit, oldLimitBuffer.data(), oldLimitBuffer.size());
            if(vm_->logSyscalls()) fmt::print(" = {} (oldLimit={}/{})\n", ret, oldLimit.rlim_cur, oldLimit.rlim_max);
            mmu_->copyToMmu(old_limit, oldLimitBuffer.data(), oldLimitBuffer.size());
            return 0;
        }
    }

    ssize_t Sys::getrandom(Ptr buf, size_t len, int flags) {
        if(vm_->logSyscalls()) 
            fmt::print("Sys::getrandom(buf={:#x}, len={}, flags={})", buf.address(), len, flags);
        std::vector<u8> buffer(len);
        std::iota(buffer.begin(), buffer.end(), 0);
        mmu_->copyToMmu(buf, buffer.data(), buffer.size());
        return (ssize_t)len;
    }

}