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
            case 0x6: { // lstat
                Ptr8 pathname{arg0};
                Ptr8 statbufptr{arg1};
                int ret = lstat(pathname, statbufptr);
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
            case 0x11: { // pread64
                i32 fd = (i32)arg0;
                Ptr8 buf{arg1};
                size_t count = (size_t)arg2;
                off_t offset = (off_t)arg3;
                ssize_t nbytes = pread64(fd, buf, count, offset);
                vm_->set(R64::RAX, (u64)nbytes);
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
            case 0x17: { // select
                int nfds = (int)arg0;
                Ptr readfds{arg1};
                Ptr writefds{arg2};
                Ptr exceptfds{arg3};
                Ptr timeout{arg4};
                int ret = select(nfds, readfds, writefds, exceptfds, timeout);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x20: { // dup
                int oldfd = (int)arg0;
                int ret = dup(oldfd);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x27: { // getpid
                vm_->set(R64::RAX, (u64)0xface);
                return;
            }
            case 0x29: { // socket
                int domain = (int)arg0;
                int type = (int)arg1;
                int protocol = (int)arg2;
                int ret = socket(domain, type, protocol);
                vm_->set(R64::RAX, (u64)ret);
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
            case 0x53: { // mkdir
                Ptr pathname {arg0};
                mode_t mode = (mode_t)arg1;
                int ret = mkdir(pathname, mode);
                vm_->set(R64::RAX, (u64)ret);
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
            case 0x66: { //getuid
                int ret = getuid();
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x68: { //getgid
                int ret = getgid();
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x6b: { //geteuid
                int ret = geteuid();
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x6c: { //getegid
                int ret = getegid();
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0x89: { // statfs
                Ptr path{arg0};
                Ptr buf{arg1};
                int ret = statfs(path, buf);
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
            case 0xbf: { // getxattr
                Ptr path{arg0};
                Ptr name{arg1};
                Ptr value{arg2};
                size_t size = (size_t)arg3;
                ssize_t ret = getxattr(path, name, value, size);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xc0: { // lgetxattr
                Ptr path{arg0};
                Ptr name{arg1};
                Ptr value{arg2};
                size_t size = (size_t)arg3;
                ssize_t ret = lgetxattr(path, name, value, size);
                vm_->set(R64::RAX, (u64)ret);
                return;
            }
            case 0xc9: { // time
                Ptr tloc{arg0};
                time_t ret = time(tloc);
                vm_->set(R64::RAX, (u64)ret);
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
            case 0xd9: { // getdents64
                int fd = (int)arg0;
                Ptr dirp{arg1};
                size_t count = (size_t)arg2;
                ssize_t ret = getdents64(fd, dirp, count);
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
            case 0x10e: { // pselect6
                int nfds = (int)arg0;
                Ptr readfds{arg1};
                Ptr writefds{arg2};
                Ptr exceptfds{arg3};
                Ptr timeout{arg4};
                Ptr sigmask{arg4};
                int ret = pselect6(nfds, readfds, writefds, exceptfds, timeout, sigmask);
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
            case 0x14c: { // statx
                int dirfd = (int)arg0;
                Ptr8 pathname{arg1};
                int flags = (int)arg2;
                unsigned int mask = (unsigned int)arg3;
                Ptr statxbuf{arg4};
                int ret = statx(dirfd, pathname, flags, mask, statxbuf);
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
        auto bufferOrErrno = Host::read(Host::FD{fd}, count);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::read(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count,
                        bufferOrErrno.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::write(int fd, Ptr8 buf, size_t count) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        ssize_t ret = Host::write(Host::FD{fd}, buffer.data(), buffer.size());
        if(vm_->logSyscalls()) {
            fmt::print("Sys::write(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count, ret);
        }
        return ret;
    }

    int Sys::close(int fd) {
        int ret = Host::close(Host::FD{fd});
        if(vm_->logSyscalls()) fmt::print("Sys::close(fd={}) = {}\n", fd, ret);
        return ret;
    }

    int Sys::stat(Ptr pathname, Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        auto bufferOrErrno = Host::stat(path);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::stat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        BufferOrErrno bufferOrErrno = Host::fstat(Host::FD{fd});
        if(vm_->logSyscalls()) {
            fmt::print("Sys::fstat(fd={}, statbuf={:#x}) = {}\n",
                        fd, statbuf.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::lstat(Ptr pathname, Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        BufferOrErrno bufferOrErrno = Host::lstat(path);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::lstat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    off_t Sys::lseek(int fd, off_t offset, int whence) {
        off_t ret = Host::lseek(Host::FD{fd}, offset, whence);
        if(vm_->logSyscalls()) fmt::print("Sys::lseek(fd={}, offset={:#x}, whence={}) = {}\n", fd, offset, whence, ret);
        return ret;
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
            if(vm_->logSyscalls()) fmt::print("Sys::ioctl({}, TCGETS, {:#x})\n", fd, argp.address());
            BufferOrErrno bufferOrErrno = Host::tcgetattr(Host::FD{fd});
            return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
                mmu_->copyToMmu(argp, buffer.data(), buffer.size());
                return 0;
            });
        }
        if(request == FIOCLEX) {
            return fcntl(fd, F_SETFD, FD_CLOEXEC);
        }
        if(request == FIONCLEX) {
            return fcntl(fd, F_SETFD, 0);
        }
        if(request == TIOCGWINSZ) {
            if(vm_->logSyscalls()) fmt::print("Sys::ioctl({}, TIOCGWINSZ, {:#x})\n", fd, argp.address());
            BufferOrErrno bufferOrErrno = Host::tiocgwinsz(Host::FD{fd});
            return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
                mmu_->copyToMmu(argp, buffer.data(), buffer.size());
                return 0;
            });
        }
        if(request == TIOCSWINSZ) {
            if(vm_->logSyscalls()) fmt::print("Sys::ioctl({}, TIOCSWINSZ, {:#x})\n", fd, argp.address());
            return 0;
        }
        if(request == TCSETSW) {
            if(vm_->logSyscalls()) fmt::print("Sys::ioctl({}, TCSETSW, {:#x})\n", fd, argp.address());
            return 0;
        }
        verify(false, [&]() {
            fmt::print("Unsupported ioctl {:x}\n", request);
        });
        return -ENOTSUP;
    }

    ssize_t Sys::pread64(int fd, Ptr buf, size_t count, off_t offset) {
        auto bufferOrErrno = Host::pread64(Host::FD{fd}, count, offset);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::read(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset,
                        bufferOrErrno.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
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

    int Sys::dup(int oldfd) {
        Host::FD newfd = Host::dup(Host::FD{oldfd});
        if(vm_->logSyscalls()) fmt::print("Sys::dup(oldfd={}) = {}\n", oldfd, newfd.fd);
        return newfd.fd;
    }

    int Sys::select(int nfds, Ptr readfds, Ptr writefds, Ptr exceptfds, Ptr timeout) {
        fd_set rfds;
        if(readfds.address() != 0) mmu_->copyFromMmu((u8*)&rfds, readfds, sizeof(fd_set));
        fd_set wfds;
        if(writefds.address() != 0) mmu_->copyFromMmu((u8*)&wfds, writefds, sizeof(fd_set));
        fd_set efds;
        if(exceptfds.address() != 0) mmu_->copyFromMmu((u8*)&efds, exceptfds, sizeof(fd_set));
        timeval to;
        if(timeout.address() != 0) mmu_->copyFromMmu((u8*)&to, timeout, sizeof(timeval));
        int ret = Host::select(nfds,
                               readfds.address() != 0 ?   &rfds : nullptr,
                               writefds.address() != 0 ?  &wfds : nullptr,
                               exceptfds.address() != 0 ? &efds : nullptr,
                               timeout.address() != 0 ?   &to : nullptr);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::select(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x}) = {}\n",
                        nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), ret);
        }
        if(readfds.address() != 0) mmu_->copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(writefds.address() != 0) mmu_->copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(exceptfds.address() != 0) mmu_->copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(timeout.address() != 0) mmu_->copyToMmu(timeout, (const u8*)&to, sizeof(timeval));
        return ret;
    }

    int Sys::socket([[maybe_unused]] int domain, [[maybe_unused]] int type, [[maybe_unused]] int protocol) {
        if(vm_->logSyscalls()) {
            fmt::print("Sys::socket(domain={}, type={}, protocol={}) = {}\n",
                                    domain, type, protocol, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    int Sys::uname(Ptr buf) {
        BufferOrErrno bufferOrErrno = Host::uname();
        if(vm_->logSyscalls()) {
            fmt::print("Sys::uname(buf={:#x}) = {}\n",
                        buf.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
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
        BufferOrErrno bufferOrErrno = Host::getcwd(size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getcwd(buf={:#x}, size={}) = {:#x}\n",
                        buf.address(), size, bufferOrErrno.isError() ? 0 : buf.address());
        }
        bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
        return bufferOrErrno.isError() ? Ptr{0x0} : buf;
    }

    int Sys::mkdir([[maybe_unused]] Ptr pathname, [[maybe_unused]] mode_t mode) {
        return -ENOTSUP;
    }

    ssize_t Sys::readlink(Ptr pathname, Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto bufferOrErrno = Host::readlink(path, bufsiz);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::readlink(path={}, buf={:#x}, size={}) = {:#x}\n",
                        path, buf.address(), bufsiz, bufferOrErrno.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::sysinfo(Ptr info) {
        auto bufferOrErrno = Host::sysinfo();
        if(vm_->logSyscalls()) {
            fmt::print("Sys::sysinfo(info={:#x}) = {}\n",
                        info.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(info, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::getuid() {
        return Host::getuid();
    }

    int Sys::getgid() {
        return Host::getgid();
    }

    int Sys::geteuid() {
        return Host::geteuid();
    }

    int Sys::getegid() {
        return Host::getegid();
    }

    int Sys::statfs(Ptr pathname, Ptr buf) {
        std::string path = mmu_->readString(pathname);
        auto bufferOrErrno = Host::statfs(path);
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    void Sys::exit_group(int status) {
        (void)status;
        if(vm_->logSyscalls()) fmt::print("Sys::exit_group(status={})\n", status);
        vm_->stop();
    }

    ssize_t Sys::getxattr(Ptr path, Ptr name, Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto bufferOrErrno = Host::getxattr(spath, sname, size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, bufferOrErrno.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::lgetxattr(Ptr path, Ptr name, Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto bufferOrErrno = Host::lgetxattr(spath, sname, size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, bufferOrErrno.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    time_t Sys::time(Ptr tloc) {
        time_t t = Host::time();
        if(vm_->logSyscalls()) fmt::print("Sys::time({:#x}) = {}\n", tloc.address(), t);
        if(tloc.address()) mmu_->copyToMmu(tloc, (const u8*)&t, sizeof(t));
        return t;
    }

    long Sys::futex(Ptr32 uaddr, int futex_op, uint32_t val, Ptr timeout, Ptr32 uaddr2, uint32_t val3) {
        if(vm_->logSyscalls()) fmt::print("Sys::futex({:#x}, {}, {}, {:#x}, {:#x}, {})\n", uaddr.address(), futex_op, val, timeout.address(), uaddr2.address(), val3);
        return 1;
    }

    ssize_t Sys::getdents64(int fd, Ptr dirp, size_t count) {
        auto bufferOrErrno = Host::getdents64(Host::FD{fd}, count);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getdents64(fd={}, dirp={:#x}, count={}) = {}\n",
                        fd, dirp.address(), count, bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return bufferOrErrno.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(dirp, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    pid_t Sys::set_tid_address(Ptr32 ptr) {
        if(vm_->logSyscalls()) fmt::print("Sys::set_tid_address({:#x}) = {}\n", ptr.address(), 1);
        return 1;
    }

    int Sys::clock_gettime(clockid_t clockid, Ptr tp) {
        auto bufferOrErrno = Host::clock_gettime(clockid);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::clock_gettime({}, {:#x}) = {}\n",
                        clockid, tp.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(tp, buffer.data(), buffer.size());
            return 0;
        });
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

    int Sys::pselect6(int nfds, Ptr readfds, Ptr writefds, Ptr exceptfds, Ptr timeout, Ptr sigmask) {
        fd_set rfds;
        if(readfds.address() != 0) mmu_->copyFromMmu((u8*)&rfds, readfds, sizeof(fd_set));
        fd_set wfds;
        if(writefds.address() != 0) mmu_->copyFromMmu((u8*)&wfds, writefds, sizeof(fd_set));
        fd_set efds;
        if(exceptfds.address() != 0) mmu_->copyFromMmu((u8*)&efds, exceptfds, sizeof(fd_set));
        timespec ts;
        if(timeout.address() != 0) mmu_->copyFromMmu((u8*)&ts, timeout, sizeof(timespec));
        sigset_t smask;
        if(sigmask.address() != 0) mmu_->copyFromMmu((u8*)&smask, sigmask, sizeof(sigset_t));
        int ret = Host::pselect6(nfds,
                               readfds.address() != 0 ?   &rfds : nullptr,
                               writefds.address() != 0 ?  &wfds : nullptr,
                               exceptfds.address() != 0 ? &efds : nullptr,
                               timeout.address() != 0 ?   &ts : nullptr,
                               sigmask.address() != 0 ?   &smask : nullptr);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::pselect6(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x},sigmask={:#x}) = {}\n",
                        nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), sigmask.address(), ret);
        }
        if(readfds.address() != 0) mmu_->copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(writefds.address() != 0) mmu_->copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(exceptfds.address() != 0) mmu_->copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(timeout.address() != 0) mmu_->copyToMmu(timeout, (const u8*)&ts, sizeof(timespec));
        return ret;
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
            fmt::print("Sys::getrandom(buf={:#x}, len={}, flags={})\n", buf.address(), len, flags);
        std::vector<u8> buffer(len);
        std::iota(buffer.begin(), buffer.end(), 0);
        mmu_->copyToMmu(buf, buffer.data(), buffer.size());
        return (ssize_t)len;
    }

    int Sys::statx(int dirfd, Ptr pathname, int flags, unsigned int mask, Ptr statxbuf) {
        std::string path = mmu_->readString(pathname);
        auto bufferOrErrno = Host::statx(Host::FD{dirfd}, path, flags, mask);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::statx(dirfd={}, path={}, flags={}, mask={}, statxbuf={:#x}) = {}\n",
                        dirfd, path, flags, mask, statxbuf.address(), bufferOrErrno.errorOr(0));
        }
        return bufferOrErrno.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statxbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

}