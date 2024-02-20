#include "interpreter/syscalls.h"
#include "interpreter/vm.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "utils/host.h"
#include <numeric>

namespace x64 {

    void Sys::syscall() {
        u64 sysNumber = vm_->get(R64::RAX);

        RegisterDump regs {{
            vm_->get(R64::RDI),
            vm_->get(R64::RSI),
            vm_->get(R64::RDX),
            vm_->get(R64::R10),
            vm_->get(R64::R8),
            vm_->get(R64::R9),
        }};

        switch(sysNumber) {
            case 0x0: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::read, regs));
            case 0x1: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::write, regs));
            case 0x3: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::close, regs));
            case 0x4: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::stat, regs));
            case 0x5: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::fstat, regs));
            case 0x6: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::lstat, regs));
            case 0x8: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::lseek, regs));
            case 0x9: return vm_->set(R64::RAX, invoke_syscall_6(&Sys::mmap, regs));
            case 0xa: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::mprotect, regs));
            case 0xb: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::munmap, regs));
            case 0xc: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::brk, regs));
            case 0xd: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::rt_sigaction, regs));
            case 0xe: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::rt_sigprocmask, regs));
            case 0x10: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::ioctl, regs));
            case 0x11: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::pread64, regs));
            case 0x14: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::writev, regs));
            case 0x15: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::access, regs));
            case 0x17: return vm_->set(R64::RAX, invoke_syscall_5(&Sys::select, regs));
            case 0x20: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::dup, regs));
            case 0x26: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::setitimer, regs));
            case 0x27: { // getpid
                vm_->set(R64::RAX, (u64)0xface);
                return;
            }
            case 0x29: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::socket, regs));
            case 0x2a: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::connect, regs));
            case 0x33: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::getsockname, regs));
            case 0x34: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::getpeername, regs));
            case 0x3f: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::uname, regs));
            case 0x48: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::fcntl, regs));
            case 0x4f: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::getcwd, regs));
            case 0x50: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::chdir, regs));
            case 0x53: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::mkdir, regs));
            case 0x59: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::readlink, regs));
            case 0x60: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::gettimeofday, regs));
            case 0x63: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::sysinfo, regs));
            case 0x66: return vm_->set(R64::RAX, invoke_syscall_0(&Sys::getuid, regs));
            case 0x68: return vm_->set(R64::RAX, invoke_syscall_0(&Sys::getgid, regs));
            case 0x6b: return vm_->set(R64::RAX, invoke_syscall_0(&Sys::geteuid, regs));
            case 0x6c: return vm_->set(R64::RAX, invoke_syscall_0(&Sys::getegid, regs));
            case 0x89: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::statfs, regs));
            case 0x9e: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::arch_prctl, regs));
            case 0xba: { // gettid
                vm_->set(R64::RAX, (u64)0xfeed);
                return;
            }
            case 0xbf: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::getxattr, regs));
            case 0xc0: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::lgetxattr, regs));
            case 0xc9: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::time, regs));
            case 0xca: return vm_->set(R64::RAX, invoke_syscall_6(&Sys::futex, regs));
            case 0xd9: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::getdents64, regs));
            case 0xda: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::set_tid_address, regs));
            case 0xdd: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::posix_fadvise, regs));
            case 0xe4: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::clock_gettime, regs));
            case 0xe7: return vm_->set(R64::RAX, invoke_syscall_1(&Sys::exit_group, regs));
            case 0xea: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::tgkill, regs));
            case 0x101: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::openat, regs));
            case 0x10e: return vm_->set(R64::RAX, invoke_syscall_6(&Sys::pselect6, regs));
            case 0x111: return vm_->set(R64::RAX, invoke_syscall_2(&Sys::set_robust_list, regs));
            case 0x112: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::get_robust_list, regs));
            case 0x118: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::utimensat, regs));
            case 0x12e: return vm_->set(R64::RAX, invoke_syscall_4(&Sys::prlimit64, regs));
            case 0x13e: return vm_->set(R64::RAX, invoke_syscall_3(&Sys::getrandom, regs));
            case 0x14c: return vm_->set(R64::RAX, invoke_syscall_5(&Sys::statx, regs));
            default: break;
        }
        verify(false, [&]() {
            fmt::print("Syscall {:#x} not handled\n", sysNumber);
        });
    }

    ssize_t Sys::read(int fd, Ptr8 buf, size_t count) {
        auto errnoOrBuffer = Host::read(Host::FD{fd}, count);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::read(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
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
        auto errnoOrBuffer = Host::stat(path);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::stat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        ErrnoOrBuffer errnoOrBuffer = Host::fstat(Host::FD{fd});
        if(vm_->logSyscalls()) {
            fmt::print("Sys::fstat(fd={}, statbuf={:#x}) = {}\n",
                        fd, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::lstat(Ptr pathname, Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        ErrnoOrBuffer errnoOrBuffer = Host::lstat(path);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::lstat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
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
        MAP f =  MAP::PRIVATE;
        if(Host::Mmap::isAnonymous(flags)) f = f | MAP::ANONYMOUS;
        if(Host::Mmap::isFixed(flags)) f = f | MAP::FIXED;
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
        // We need to ask the host for the expected buffer size behind argp.
        size_t bufferSize = Host::ioctlRequiredBufferSize(request);
        std::vector<u8> buf(bufferSize, 0x0);
        Buffer buffer(std::move(buf));
        mmu_->copyFromMmu(buffer.data(), argp, buffer.size());
        auto errnoOrBuffer = Host::ioctl(Host::FD{fd}, request, buffer);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::ioctl(fd={}, request={}, argp={:#x}) = {}\n",
                        fd, Host::ioctlName(request), argp.address(),
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            // The buffer returned by ioctl is empty when nothing needs to be written back.
            mmu_->copyToMmu(argp, buffer.data(), buffer.size());
            return 0;
        });
    }

    ssize_t Sys::pread64(int fd, Ptr buf, size_t count, off_t offset) {
        auto errnoOrBuffer = Host::pread64(Host::FD{fd}, count, offset);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::read(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::writev(int fd, Ptr iov, int iovcnt) {
        std::vector<u8> iovecs = mmu_->readFromMmu<u8>(iov, ((size_t)iovcnt) * Host::iovecRequiredBufferSize());
        Buffer iovecBuffer(std::move(iovecs));
        ssize_t nbytes = Host::writev(Host::FD{fd}, iovecBuffer);
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

    int Sys::setitimer([[maybe_unused]] int which, [[maybe_unused]]const Ptr new_value, [[maybe_unused]]Ptr old_value) {
        return 0;
    }

    int Sys::select(int nfds, Ptr readfds, Ptr writefds, Ptr exceptfds, Ptr timeout) {
        fd_set rfds;
        fd_set wfds;
        fd_set efds;
        timeval to;
        if(!!readfds) mmu_->copyFromMmu((u8*)&rfds, readfds, sizeof(fd_set));
        if(!!writefds) mmu_->copyFromMmu((u8*)&wfds, writefds, sizeof(fd_set));
        if(!!exceptfds) mmu_->copyFromMmu((u8*)&efds, exceptfds, sizeof(fd_set));
        if(!!timeout) mmu_->copyFromMmu((u8*)&to, timeout, sizeof(timeval));
        int ret = Host::select(nfds,
                               !!readfds ?   &rfds : nullptr,
                               !!writefds ?  &wfds : nullptr,
                               !!exceptfds ? &efds : nullptr,
                               !!timeout ?   &to : nullptr);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::select(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x}) = {}\n",
                        nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), ret);
        }
        if(!!readfds) mmu_->copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(!!writefds) mmu_->copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(!!exceptfds) mmu_->copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(!!timeout) mmu_->copyToMmu(timeout, (const u8*)&to, sizeof(timeval));
        return ret;
    }

    int Sys::socket(int domain, int type, int protocol) {
        Host::FD fd = Host::socket(domain, type, protocol);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::socket(domain={}, type={}, protocol={}) = {}\n",
                                    domain, type, protocol, fd.fd);
        }
        return fd.fd;
    }

    int Sys::connect(int sockfd, Ptr addr, size_t addrlen) {
        std::vector<u8> addrBuffer = mmu_->readFromMmu<u8>(addr, (size_t)addrlen);
        Buffer buf(std::move(addrBuffer));
        int ret = Host::connect(sockfd, buf);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::connect(sockfd={}, addr={:#x}, addrlen={}) = {}\n",
                        sockfd, addr.address(), addrlen, ret);
        }
        return ret;
    }

    int Sys::getsockname(int sockfd, Ptr addr, Ptr32 addrlen) {
        u32 buffersize = mmu_->read32(addrlen);
        ErrnoOrBuffer sockname = Host::getsockname(sockfd, buffersize);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getsockname(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen, sockname.errorOr(0));
            sockname.errorOrWith<int>([&](const auto& buffer) {
                fmt::print("{}\n", (const char*)buffer.data());
                return 0;
            });
        }
        return sockname.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(addr, buffer.data(), buffer.size());
            mmu_->write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::getpeername(int sockfd, Ptr addr, Ptr32 addrlen) {
        u32 buffersize = mmu_->read32(addrlen);
        ErrnoOrBuffer peername = Host::getpeername(sockfd, buffersize);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getpeername(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen, peername.errorOr(0));
            peername.errorOrWith<int>([&](const auto& buffer) {
                fmt::print("{}\n", (const char*)buffer.data());
                return 0;
            });
        }
        return peername.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(addr, buffer.data(), buffer.size());
            mmu_->write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::uname(Ptr buf) {
        ErrnoOrBuffer errnoOrBuffer = Host::uname();
        if(vm_->logSyscalls()) {
            fmt::print("Sys::uname(buf={:#x}) = {}\n",
                        buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fcntl(int fd, int cmd, int arg) {
        int ret = Host::fcntl(Host::FD{fd}, cmd, arg);
        if(vm_->logSyscalls()) fmt::print("Sys::fcntl(fd={}, cmd={}, arg={}) = {}\n", fd, Host::fcntlName(cmd), arg, ret);
        return ret;
    }

    Ptr Sys::getcwd(Ptr buf, size_t size) {
        ErrnoOrBuffer errnoOrBuffer = Host::getcwd(size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getcwd(buf={:#x}, size={}) = {:#x}\n",
                        buf.address(), size, errnoOrBuffer.isError() ? 0 : buf.address());
        }
        errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
        return errnoOrBuffer.isError() ? Ptr{0x0} : buf;
    }

    int Sys::chdir(Ptr pathname) {
        auto path = mmu_->readString(pathname);
        int ret = Host::chdir(path);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::chdir(path={}) = {}\n", path, ret);
        }
        return ret;
    }

    int Sys::mkdir([[maybe_unused]] Ptr pathname, [[maybe_unused]] mode_t mode) {
        return -ENOTSUP;
    }

    ssize_t Sys::readlink(Ptr pathname, Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::readlink(path, bufsiz);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::readlink(path={}, buf={:#x}, size={}) = {:#x}\n",
                        path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::gettimeofday(Ptr tv, Ptr tz) {
        auto errnoOrBuffers = Host::gettimeofday();
        if(vm_->logSyscalls()) {
            fmt::print("Sys::gettimeofday(tv={:#x}, tz={:#x}) = {:#x}\n",
                        tv.address(), tz.address(), errnoOrBuffers.errorOr(0));
        }
        return errnoOrBuffers.errorOrWith<int>([&](const auto& buffers) {
            if(!!tv) mmu_->copyToMmu(tv, buffers.first.data(), buffers.first.size());
            if(!!tz) mmu_->copyToMmu(tz, buffers.second.data(), buffers.second.size());
            return 0;
        });
    }

    int Sys::sysinfo(Ptr info) {
        auto errnoOrBuffer = Host::sysinfo();
        if(vm_->logSyscalls()) {
            fmt::print("Sys::sysinfo(info={:#x}) = {}\n",
                        info.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
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
        auto errnoOrBuffer = Host::statfs(path);
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    u64 Sys::exit_group(int status) {
        (void)status;
        if(vm_->logSyscalls()) fmt::print("Sys::exit_group(status={})\n", status);
        vm_->stop();
        return 0;
    }

    int Sys::tgkill(int tgid, int tid, int sig) {
        if(vm_->logSyscalls()) fmt::print("Sys::tgkill(tgid={}, tid={}, sig={})\n", tgid, tid, sig);
        vm_->stop();
        return 0;
    }

    ssize_t Sys::getxattr(Ptr path, Ptr name, Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto errnoOrBuffer = Host::getxattr(spath, sname, size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::lgetxattr(Ptr path, Ptr name, Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto errnoOrBuffer = Host::lgetxattr(spath, sname, size);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
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
        auto errnoOrBuffer = Host::getdents64(Host::FD{fd}, count);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::getdents64(fd={}, dirp={:#x}, count={}) = {}\n",
                        fd, dirp.address(), count, errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(dirp, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    pid_t Sys::set_tid_address(Ptr32 ptr) {
        if(vm_->logSyscalls()) fmt::print("Sys::set_tid_address({:#x}) = {}\n", ptr.address(), 1);
        return 1;
    }

    int Sys::posix_fadvise([[maybe_unused]] int fd, [[maybe_unused]] off_t offset, [[maybe_unused]] off_t len, [[maybe_unused]] int advice) {
        return 0;
    }

    int Sys::clock_gettime(clockid_t clockid, Ptr tp) {
        auto errnoOrBuffer = Host::clock_gettime(clockid);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::clock_gettime({}, {:#x}) = {}\n",
                        clockid, tp.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(tp, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::arch_prctl(int code, Ptr addr) {
        bool isSetFS = Host::Prctl::isSetFS(code);
        if(vm_->logSyscalls()) fmt::print("Sys::arch_prctl(code={}, addr={:#x}) = {}\n", code, addr.address(), isSetFS ? 0 : -EINVAL);
        if(!isSetFS) return -EINVAL;
        mmu_->setSegmentBase(Segment::FS, addr.address());
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

    int Sys::utimensat(int dirfd, Ptr pathname, Ptr times, int flags) {
        if(vm_->logSyscalls()) fmt::print("Sys::utimensat(dirfd={}, pathname={}, times={:#x}, flags={}) = -ENOTSUP\n",
                                                          dirfd, mmu_->readString(pathname), times.address(), flags);
        return -ENOTSUP;
    }

    int Sys::prlimit64(pid_t pid, int resource, [[maybe_unused]] Ptr new_limit, Ptr old_limit) {
        if(vm_->logSyscalls()) 
            fmt::print("Sys::prlimit64(pid={}, resource={}, new_limit={:#x}, old_limit={:#x})", pid, resource, new_limit.address(), old_limit.address());
        if(!old_limit.address()) {
            if(vm_->logSyscalls()) fmt::print(" = 0\n");
            return 0;
        }
        auto errnoOrBuffer = Host::getrlimit(pid, resource);
        if(vm_->logSyscalls()) fmt::print(" = {}\n", errnoOrBuffer.errorOr(0));
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(old_limit, buffer.data(), buffer.size());
            return 0;
        });
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
        auto errnoOrBuffer = Host::statx(Host::FD{dirfd}, path, flags, mask);
        if(vm_->logSyscalls()) {
            fmt::print("Sys::statx(dirfd={}, path={}, flags={}, mask={}, statxbuf={:#x}) = {}\n",
                        dirfd, path, flags, mask, statxbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statxbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

}