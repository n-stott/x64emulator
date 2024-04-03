#include "interpreter/syscalls.h"
#include "interpreter/interpreter.h"
#include "interpreter/scheduler.h"
#include "interpreter/thread.h"
#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "utils/host.h"
#include <numeric>
#include <sys/socket.h>

namespace x64 {

    template<typename... Args>
    void Sys::print(const char* format, Args... args) const {
        fmt::print("[{}:{}] ", scheduler_->currentThread()->descr.pid, scheduler_->currentThread()->descr.tid);
        fmt::print(format, args...);
    }

    void Sys::syscall(Cpu* cpu) {
        u64 sysNumber = cpu->get(R64::RAX);

        RegisterDump regs {{
            cpu->get(R64::RDI),
            cpu->get(R64::RSI),
            cpu->get(R64::RDX),
            cpu->get(R64::R10),
            cpu->get(R64::R8),
            cpu->get(R64::R9),
        }};

        switch(sysNumber) {
            case 0x0: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::read, regs));
            case 0x1: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::write, regs));
            case 0x3: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::close, regs));
            case 0x4: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::stat, regs));
            case 0x5: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::fstat, regs));
            case 0x6: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::lstat, regs));
            case 0x7: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::poll, regs));
            case 0x8: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::lseek, regs));
            case 0x9: return cpu->set(R64::RAX, invoke_syscall_6(&Sys::mmap, regs));
            case 0xa: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::mprotect, regs));
            case 0xb: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::munmap, regs));
            case 0xc: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::brk, regs));
            case 0xd: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::rt_sigaction, regs));
            case 0xe: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::rt_sigprocmask, regs));
            case 0x10: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::ioctl, regs));
            case 0x11: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::pread64, regs));
            case 0x12: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::pwrite64, regs));
            case 0x14: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::writev, regs));
            case 0x15: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::access, regs));
            case 0x17: return cpu->set(R64::RAX, invoke_syscall_5(&Sys::select, regs));
            case 0x1c: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::madvise, regs));
            case 0x20: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::dup, regs));
            case 0x26: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::setitimer, regs));
            case 0x27: { // getpid
                verify(!!scheduler_->currentThread());
                cpu->set(R64::RAX, (u64)scheduler_->currentThread()->descr.pid);
                return;
            }
            case 0x29: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::socket, regs));
            case 0x2a: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::connect, regs));
            case 0x2d: return cpu->set(R64::RAX, invoke_syscall_6(&Sys::recvfrom, regs));
            case 0x2e: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::sendmsg, regs));
            case 0x2f: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::recvmsg, regs));
            case 0x31: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::bind, regs));
            case 0x33: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::getsockname, regs));
            case 0x34: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::getpeername, regs));
            case 0x38: return cpu->set(R64::RAX, invoke_syscall_5(&Sys::clone, regs));
            case 0x3c: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::exit, regs));
            case 0x3f: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::uname, regs));
            case 0x48: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::fcntl, regs));
            case 0x4a: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::fsync, regs));
            case 0x4f: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::getcwd, regs));
            case 0x50: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::chdir, regs));
            case 0x53: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::mkdir, regs));
            case 0x57: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::unlink, regs));
            case 0x59: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::readlink, regs));
            case 0x60: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::gettimeofday, regs));
            case 0x63: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::sysinfo, regs));
            case 0x66: return cpu->set(R64::RAX, invoke_syscall_0(&Sys::getuid, regs));
            case 0x68: return cpu->set(R64::RAX, invoke_syscall_0(&Sys::getgid, regs));
            case 0x6b: return cpu->set(R64::RAX, invoke_syscall_0(&Sys::geteuid, regs));
            case 0x6c: return cpu->set(R64::RAX, invoke_syscall_0(&Sys::getegid, regs));
            case 0x6f: return cpu->set(R64::RAX, invoke_syscall_0(&Sys::getpgrp, regs));
            case 0x89: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::statfs, regs));
            case 0x9d: return cpu->set(R64::RAX, invoke_syscall_5(&Sys::prctl, regs));
            case 0x9e: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::arch_prctl, regs));
            case 0xba: { // gettid
                verify(!!scheduler_->currentThread());
                cpu->set(R64::RAX, (u64)scheduler_->currentThread()->descr.tid);
                return;
            }
            case 0xbf: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::getxattr, regs));
            case 0xc0: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::lgetxattr, regs));
            case 0xc9: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::time, regs));
            case 0xcc: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::sched_getaffinity, regs));
            case 0xca: return cpu->set(R64::RAX, invoke_syscall_6(&Sys::futex, regs));
            case 0xd9: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::getdents64, regs));
            case 0xda: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::set_tid_address, regs));
            case 0xdd: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::posix_fadvise, regs));
            case 0xe4: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::clock_gettime, regs));
            case 0xe7: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::exit_group, regs));
            case 0xea: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::tgkill, regs));
            case 0x101: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::openat, regs));
            case 0x106: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::fstatat64, regs));
            case 0x10e: return cpu->set(R64::RAX, invoke_syscall_6(&Sys::pselect6, regs));
            case 0x111: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::set_robust_list, regs));
            case 0x112: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::get_robust_list, regs));
            case 0x118: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::utimensat, regs));
            case 0x122: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::eventfd2, regs));
            case 0x123: return cpu->set(R64::RAX, invoke_syscall_1(&Sys::epoll_create1, regs));
            case 0x12e: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::prlimit64, regs));
            case 0x13b: return cpu->set(R64::RAX, invoke_syscall_4(&Sys::sched_getattr, regs));
            case 0x13e: return cpu->set(R64::RAX, invoke_syscall_3(&Sys::getrandom, regs));
            case 0x14c: return cpu->set(R64::RAX, invoke_syscall_5(&Sys::statx, regs));
            case 0x1b3: return cpu->set(R64::RAX, invoke_syscall_2(&Sys::clone3, regs));
            default: break;
        }
        verify(false, [&]() {
            print("Syscall {:#x} not handled\n", sysNumber);
        });
    }

    ssize_t Sys::read(int fd, Ptr8 buf, size_t count) {
        auto errnoOrBuffer = Host::read(Host::FD{fd}, count);
        if(logSyscalls_) {
            print("Sys::read(fd={}, buf={:#x}, count={}) = {}\n",
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
        if(logSyscalls_) {
            print("Sys::write(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count, ret);
        }
        return ret;
    }

    int Sys::close(int fd) {
        int ret = Host::close(Host::FD{fd});
        if(logSyscalls_) print("Sys::close(fd={}) = {}\n", fd, ret);
        return ret;
    }

    int Sys::stat(Ptr pathname, Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::stat(path);
        if(logSyscalls_) {
            print("Sys::stat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstat(int fd, Ptr8 statbuf) {
        ErrnoOrBuffer errnoOrBuffer = Host::fstat(Host::FD{fd});
        if(logSyscalls_) {
            print("Sys::fstat(fd={}, statbuf={:#x}) = {}\n",
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
        if(logSyscalls_) {
            print("Sys::lstat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::poll(Ptr fds, size_t nfds, int timeout) {
        std::vector<u8> pollfds = mmu_->readFromMmu<u8>(fds, Host::pollRequiredBufferSize(nfds));
        Buffer buffer(std::move(pollfds));
        auto errnoOrBufferAndReturnValue = Host::poll(buffer, nfds, timeout);
        if(logSyscalls_) {
            print("Sys::poll(fds={:#x}, nfds={}, timeout={}) = {}\n",
                                  fds.address(), nfds, timeout,
                                  errnoOrBufferAndReturnValue.errorOrWith<int>([](const auto& bufferAndRetVal) {
                                    return bufferAndRetVal.returnValue;
                                  }));
        }
        return errnoOrBufferAndReturnValue.errorOrWith<int>([&](const auto& bufferAndRetVal) {
            mmu_->copyToMmu(fds, bufferAndRetVal.buffer.data(), bufferAndRetVal.buffer.size());
            return bufferAndRetVal.returnValue;
        });
    }

    off_t Sys::lseek(int fd, off_t offset, int whence) {
        off_t ret = Host::lseek(Host::FD{fd}, offset, whence);
        if(logSyscalls_) print("Sys::lseek(fd={}, offset={:#x}, whence={}) = {}\n", fd, offset, whence, ret);
        return ret;
    }

    Ptr Sys::mmap(Ptr addr, size_t length, int prot, int flags, int fd, off_t offset) {
        MAP f =  MAP::PRIVATE;
        if(Host::Mmap::isAnonymous(flags)) f = f | MAP::ANONYMOUS;
        if(Host::Mmap::isFixed(flags)) f = f | MAP::FIXED;
        verify(addr.segment() != Segment::FS);
        u64 base = mmu_->mmap(addr.address(), length, (PROT)prot, f, fd, (int)offset);
        if(logSyscalls_) print("Sys::mmap(addr={:#x}, length={}, prot={}, flags={}, fd={}, offset={}) = {:#x}\n",
                                              addr.address(), length, prot, flags, fd, offset, base);
        return Ptr{addr.segment(), base};
    }

    int Sys::mprotect(Ptr addr, size_t length, int prot) {
        int ret = mmu_->mprotect(addr.address(), length, (PROT)prot);
        if(logSyscalls_) print("Sys::mprotect(addr={:#x}, length={}, prot={}) = {}\n", addr.address(), length, prot, ret);
        return ret;
    }

    int Sys::munmap(Ptr addr, size_t length) {
        int ret = mmu_->munmap(addr.address(), length);
        if(logSyscalls_) print("Sys::munmap(addr={:#x}, length={}) = {}\n", addr.address(), length, ret);
        return ret;
    }

    Ptr Sys::brk(Ptr addr) {
        verify(addr.segment() != Segment::FS);
        u64 newBrk = mmu_->brk(addr.address());
        if(logSyscalls_) print("Sys::brk(addr={:#x}) = {:#x}\n", addr.address(), newBrk);
        return Ptr{addr.segment(), newBrk};
    }

    int Sys::rt_sigaction(int sig, Ptr act, Ptr oact, size_t sigsetsize) {
        if(logSyscalls_) print("Sys::rt_sigaction({}, {:#x}, {:#x}, {}) = 0\n", sig, act.address(), oact.address(), sigsetsize);
        (void)sig;
        (void)act;
        (void)oact;
        (void)sigsetsize;
        return 0;
    }

    int Sys::rt_sigprocmask(int how, Ptr nset, Ptr oset, size_t sigsetsize) {
        if(logSyscalls_) print("Sys::rt_sigprocmask({}, {:#x}, {:#x}, {}) = 0\n", how, nset.address(), oset.address(), sigsetsize);
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
        if(logSyscalls_) {
            print("Sys::ioctl(fd={}, request={}, argp={:#x}) = {}\n",
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
        if(logSyscalls_) {
            print("Sys::pread64(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::pwrite64(int fd, Ptr buf, size_t count, off_t offset) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        auto errnoOrNbytes = Host::pwrite64(Host::FD{fd}, buffer.data(), buffer.size(), offset);
        if(logSyscalls_) {
            print("Sys::pwrite64(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset, errnoOrNbytes);
        }
        return errnoOrNbytes;
    }

    ssize_t Sys::writev(int fd, Ptr iov, int iovcnt) {
        std::vector<u8> iovecs = mmu_->readFromMmu<u8>(iov, ((size_t)iovcnt) * Host::iovecRequiredBufferSize());
        Buffer iovecBuffer(std::move(iovecs));
        std::vector<Buffer> buffers;
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            Ptr base{Host::iovecBase(iovecBuffer, i)};
            size_t len = Host::iovecLen(iovecBuffer, i);
            std::vector<u8> data;
            data.resize(len);
            mmu_->copyFromMmu(data.data(), base, len);
            buffers.push_back(Buffer(std::move(data)));
        }
        ssize_t nbytes = Host::writev(Host::FD{fd}, buffers);
        if(logSyscalls_) print("Sys::writev(fd={}, iov={:#x}, iovcnt={}) = {}\n", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    int Sys::access(Ptr pathname, int mode) {
        std::string path = mmu_->readString(pathname);
        int ret = Host::access(path, mode);
        std::string info;
        if(ret < 0) {
            info = strerror(-ret);
        }
        if(logSyscalls_) print("Sys::access(path={}, mode={}) = {} {}\n", path, mode, ret, info);
        return ret;
    }

    int Sys::dup(int oldfd) {
        Host::FD newfd = Host::dup(Host::FD{oldfd});
        if(logSyscalls_) print("Sys::dup(oldfd={}) = {}\n", oldfd, newfd.fd);
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
        if(logSyscalls_) {
            print("Sys::select(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x}) = {}\n",
                        nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), ret);
        }
        if(!!readfds) mmu_->copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(!!writefds) mmu_->copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(!!exceptfds) mmu_->copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(!!timeout) mmu_->copyToMmu(timeout, (const u8*)&to, sizeof(timeval));
        return ret;
    }

    int Sys::madvise(Ptr addr, size_t length, int advice) {
        int ret = 0;
        if(logSyscalls_) {
            print("Sys::madvise(addr={:#x}, length={}, advice={}) = {}\n",
                                    addr.address(), length, advice, ret);
        }
        return ret;
    }

    int Sys::socket(int domain, int type, int protocol) {
        Host::FD fd = Host::socket(domain, type, protocol);
        if(logSyscalls_) {
            print("Sys::socket(domain={}, type={}, protocol={}) = {}\n",
                                    domain, type, protocol, fd.fd);
        }
        return fd.fd;
    }

    int Sys::connect(int sockfd, Ptr addr, size_t addrlen) {
        std::vector<u8> addrBuffer = mmu_->readFromMmu<u8>(addr, (size_t)addrlen);
        Buffer buf(std::move(addrBuffer));
        int ret = Host::connect(sockfd, buf);
        if(logSyscalls_) {
            print("Sys::connect(sockfd={}, addr={:#x}, addrlen={}) = {}\n",
                        sockfd, addr.address(), addrlen, ret);
        }
        return ret;
    }

    int Sys::getsockname(int sockfd, Ptr addr, Ptr32 addrlen) {
        u32 buffersize = mmu_->read32(addrlen);
        ErrnoOrBuffer sockname = Host::getsockname(sockfd, buffersize);
        if(logSyscalls_) {
            print("Sys::getsockname(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen.address(), sockname.errorOr(0));
            sockname.errorOrWith<int>([&](const auto& buffer) {
                print("{}\n", (const char*)buffer.data());
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
        if(logSyscalls_) {
            print("Sys::getpeername(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen.address(), peername.errorOr(0));
            peername.errorOrWith<int>([&](const auto& buffer) {
                print("{}\n", (const char*)buffer.data());
                return 0;
            });
        }
        return peername.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(addr, buffer.data(), buffer.size());
            mmu_->write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    long Sys::clone(unsigned long flags, Ptr stack, Ptr parent_tid, Ptr32 child_tid, unsigned long tls) {
        Thread* currentThread = scheduler_->currentThread();
        Thread* newThread = scheduler_->createThread(currentThread->descr.pid);
        newThread->data.regs = currentThread->data.regs;
        newThread->data.regs.rip() = currentThread->data.regs.rip();
        newThread->data.regs.set(R64::RAX, 0);
        newThread->data.regs.rsp() = stack.address();
        newThread->clear_child_tid = child_tid;
        long ret = newThread->descr.tid;
        if(!!child_tid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_->write32(child_tid, (u32)ret);
        }
        if(logSyscalls_) {
            print("Sys::clone(flags={}, stack={:#x}, parent_tid={:#x}, child_tid={:#x}, tls={}) = {}\n",
                        flags, stack.address(), parent_tid.address(), child_tid.address(), tls, ret);
        }
        return ret;
    }

    int Sys::exit(int status) {
        if(logSyscalls_) {
            print("Sys::exit(status={})\n", status);
        }
        Thread* thread = scheduler_->currentThread();
        thread->yield();
        scheduler_->terminate(thread, status);
        return status;
    }

    int Sys::uname(Ptr buf) {
        ErrnoOrBuffer errnoOrBuffer = Host::uname();
        if(logSyscalls_) {
            print("Sys::uname(buf={:#x}) = {}\n",
                        buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fcntl(int fd, int cmd, int arg) {
        int ret = Host::fcntl(Host::FD{fd}, cmd, arg);
        if(logSyscalls_) print("Sys::fcntl(fd={}, cmd={}, arg={}) = {}\n", fd, Host::fcntlName(cmd), arg, ret);
        return ret;
    }

    int Sys::fsync(int fd) {
        if(logSyscalls_) print("Sys::fsync(fd={}) = {}\n", fd, -EINVAL);
        return -EINVAL;
    }

    Ptr Sys::getcwd(Ptr buf, size_t size) {
        ErrnoOrBuffer errnoOrBuffer = Host::getcwd(size);
        if(logSyscalls_) {
            print("Sys::getcwd(buf={:#x}, size={}) = {:#x}\n",
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
        if(logSyscalls_) {
            print("Sys::chdir(path={}) = {}\n", path, ret);
        }
        return ret;
    }

    int Sys::mkdir([[maybe_unused]] Ptr pathname, [[maybe_unused]] mode_t mode) {
        if(logSyscalls_) {
            auto path = mmu_->readString(pathname);
            print("Sys::mkdir(path={}, mode={}) = {}\n", path, mode, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    int Sys::unlink([[maybe_unused]] Ptr pathname) {
        if(logSyscalls_) {
            auto path = mmu_->readString(pathname);
            print("Sys::unlink(path={}) = {}\n", path, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    ssize_t Sys::readlink(Ptr pathname, Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::readlink(path, bufsiz);
        if(logSyscalls_) {
            print("Sys::readlink(path={}, buf={:#x}, size={}) = {:#x}\n",
                        path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::gettimeofday(Ptr tv, Ptr tz) {
        auto errnoOrBuffers = Host::gettimeofday();
        if(logSyscalls_) {
            print("Sys::gettimeofday(tv={:#x}, tz={:#x}) = {:#x}\n",
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
        if(logSyscalls_) {
            print("Sys::sysinfo(info={:#x}) = {}\n",
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

    int Sys::getpgrp() {
        return Host::getpgrp();
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
        if(logSyscalls_) print("Sys::exit_group(status={})\n", status);
        interpreter_->stop();
        return 0;
    }

    int Sys::tgkill(int tgid, int tid, int sig) {
        if(logSyscalls_) print("Sys::tgkill(tgid={}, tid={}, sig={})\n", tgid, tid, sig);
        interpreter_->stop();
        return 0;
    }

    ssize_t Sys::getxattr(Ptr path, Ptr name, Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto errnoOrBuffer = Host::getxattr(spath, sname, size);
        if(logSyscalls_) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
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
        if(logSyscalls_) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
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
        if(logSyscalls_) print("Sys::time({:#x}) = {}\n", tloc.address(), t);
        if(tloc.address()) mmu_->copyToMmu(tloc, (const u8*)&t, sizeof(t));
        return t;
    }

    long Sys::futex(Ptr32 uaddr, int futex_op, uint32_t val, Ptr timeout, Ptr32 uaddr2, uint32_t val3) {
        if(logSyscalls_) print("Sys::futex(uaddr={:#x}, op={}, val={}, timeout={:#x}, uaddr2={:#x}, val3={})\n", uaddr.address(), futex_op, val, timeout.address(), uaddr2.address(), val3);
        if((futex_op & 0x7f) == 0) {
            // wait
            u32 loaded = mmu_->read32(uaddr);
            if(loaded != val) return -EAGAIN;
            Thread* thread = scheduler_->currentThread();
            scheduler_->wait(thread, uaddr, val);
            thread->yield();
            return 1;
        }
        if((futex_op & 0x7f) == 1) {
            // wake
            u32 nbWoken = scheduler_->wake(uaddr, val);
            scheduler_->currentThread()->yield();
            return nbWoken;
        }
        verify(false, [&]() {
            fmt::print("futex with op={} is not supported\n", futex_op);
        });
        return 1;
    }

    int Sys::sched_getaffinity(pid_t pid, size_t cpusetsize, Ptr mask) {
        int ret = 0;
        if(pid == 0) {
            // pretend that only cpu 0 is available.
            std::vector<u8> buffer;
            buffer.resize(cpusetsize, 0x0);
            if(!buffer.empty()) {
                buffer[0] |= 0x1;
            }
            mmu_->copyToMmu(mask, buffer.data(), buffer.size());
        } else {
            // don't allow looking at other processes
            ret = -EPERM;
        }
        if(logSyscalls_) print("Sys::sched_getaffinity({}, {}, {:#x}) = {}\n",
                                                                  pid, cpusetsize, mask.address(), ret);
        return ret;
    }

    ssize_t Sys::recvfrom(int sockfd, Ptr buf, size_t len, int flags, Ptr src_addr, Ptr32 addrlen) {
        bool requireSrcAddress = !!src_addr && !!addrlen;
        ErrnoOr<std::pair<Buffer, Buffer>> ret = Host::recvfrom(Host::FD{sockfd}, len, flags, requireSrcAddress);
        if(logSyscalls_) {
            print("Sys::recvfrom(sockfd={}, buf={:#x}, len={}, flags={}, src_addr={:#x}, addrlen={:#x}) = {}",
                                      sockfd, buf.address(), len, flags, src_addr.address(), addrlen.address(),
                                      ret.errorOrWith<ssize_t>([](const auto& buffers) {
                return (ssize_t)buffers.first.size();
            }));
        }
        return ret.errorOrWith<ssize_t>([&](const auto& buffers) {
            mmu_->copyToMmu(buf, buffers.first.data(), buffers.first.size());
            if(requireSrcAddress) {
                mmu_->copyToMmu(src_addr, buffers.second.data(), buffers.second.size());
                mmu_->write32(addrlen, (u32)buffers.second.size());
            }
            return (ssize_t)buffers.first.size();
        });
    }

    ssize_t Sys::sendmsg(int sockfd, Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header;
        mmu_->copyFromMmu((u8*)&header, msg, sizeof(msghdr));
        std::vector<u8> msg_name_buffer(header.msg_name ? header.msg_namelen : 0, 0x0);
        Buffer msg_name(std::move(msg_name_buffer));
        if(header.msg_name) mmu_->copyFromMmu(msg_name.data(), Ptr8{(u64)header.msg_name}, msg_name.size());

        std::vector<u8> msg_control_buffer(header.msg_control ? header.msg_controllen : 0, 0x0);
        Buffer msg_control(std::move(msg_control_buffer));
        if(header.msg_control) mmu_->copyFromMmu(msg_control.data(), Ptr8{(u64)header.msg_control}, msg_control.size());

        std::vector<Buffer> msg_iov;
        std::vector<iovec> msg_iovecs = mmu_->readFromMmu<iovec>(Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            std::vector<u8> buffer(msg_iovecs[i].iov_len, 0x0);
            msg_iov.push_back(Buffer(std::move(buffer)));
            mmu_->copyFromMmu(msg_iov[i].data(), Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iov[i].size());
        }

        int msg_flags = 0;
        ssize_t nbytes = Host::sendmsg(Host::FD{sockfd}, flags, msg_name, msg_iov, msg_control, msg_flags);
        if(logSyscalls_) {
            print("Sys::sendmsg(sockfd={}, msg={:#x}, flags={}) = {}\n",
                        sockfd, msg.address(), flags, nbytes);
        }
        if(nbytes < 0) return nbytes;
        return nbytes;
    }

    ssize_t Sys::recvmsg(int sockfd, Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header;
        mmu_->copyFromMmu((u8*)&header, msg, sizeof(msghdr));
        std::vector<u8> msg_name_buffer(header.msg_name ? header.msg_namelen : 0, 0x0);
        Buffer msg_name(std::move(msg_name_buffer));
        std::vector<u8> msg_control_buffer(header.msg_control ? header.msg_controllen : 0, 0x0);
        Buffer msg_control(std::move(msg_control_buffer));
        std::vector<Buffer> msg_iov;
        std::vector<iovec> msg_iovecs = mmu_->readFromMmu<iovec>(Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            std::vector<u8> buffer(msg_iovecs[i].iov_len, 0x0);
            msg_iov.push_back(Buffer(std::move(buffer)));
        }

        int msg_flags = 0;
        ssize_t nbytes = Host::recvmsg(Host::FD{sockfd}, flags, &msg_name, &msg_iov, &msg_control, &msg_flags);
        if(logSyscalls_) {
            print("Sys::recvmsg(sockfd={}, msg={:#x}, flags={}) = {}\n",
                        sockfd, msg.address(), flags, nbytes);
        }
        if(nbytes < 0) return nbytes;

        if(header.msg_name) mmu_->copyToMmu(Ptr8{(u64)header.msg_name}, msg_name.data(), msg_name.size());
        if(header.msg_control) mmu_->copyToMmu(Ptr8{(u64)header.msg_control}, msg_control.data(), msg_control.size());
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            mmu_->copyToMmu(Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iov[i].data(), msg_iov[i].size());
        }
        return nbytes;
    }

    int Sys::bind(int sockfd, Ptr addr, socklen_t addrlen) {
        Buffer saddr(mmu_->readFromMmu<u8>(addr, addrlen));
        int rc = Host::bind(Host::FD{sockfd}, saddr);
        if(logSyscalls_) {
            print("Sys::bind(sockfd={}, addr={:#x}, addrlen={}) = {}\n",
                        sockfd, addr.address(), addrlen, rc);
        }
        return rc;
    }

    ssize_t Sys::getdents64(int fd, Ptr dirp, size_t count) {
        auto errnoOrBuffer = Host::getdents64(Host::FD{fd}, count);
        if(logSyscalls_) {
            print("Sys::getdents64(fd={}, dirp={:#x}, count={}) = {}\n",
                        fd, dirp.address(), count, errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(dirp, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    pid_t Sys::set_tid_address(Ptr32 ptr) {
        if(logSyscalls_) print("Sys::set_tid_address({:#x}) = {}\n", ptr.address(), 1);
        return 1;
    }

    int Sys::posix_fadvise([[maybe_unused]] int fd, [[maybe_unused]] off_t offset, [[maybe_unused]] off_t len, [[maybe_unused]] int advice) {
        return 0;
    }

    int Sys::clock_gettime(clockid_t clockid, Ptr tp) {
        auto errnoOrBuffer = Host::clock_gettime(clockid);
        if(logSyscalls_) {
            print("Sys::clock_gettime({}, {:#x}) = {}\n",
                        clockid, tp.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(tp, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
        if(logSyscalls_) {
            print("Sys::prctl(option={}, arg2={}, arg3={}, arg4={}, arg5={}) = {}\n", option, arg2, arg3, arg4, arg5, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    int Sys::arch_prctl(int code, Ptr addr) {
        bool isSetFS = Host::Prctl::isSetFS(code);
        if(logSyscalls_) print("Sys::arch_prctl(code={}, addr={:#x}) = {}\n", code, addr.address(), isSetFS ? 0 : -EINVAL);
        if(!isSetFS) return -EINVAL;
        mmu_->setSegmentBase(Segment::FS, addr.address());
        return 0;
    }

    int Sys::openat(int dirfd, Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_->readString(pathname);
        Host::FD fd = Host::openat(Host::FD{dirfd}, path, flags, mode);
        if(logSyscalls_) print("Sys::openat(dirfd={}, path={}, flags={}, mode={}) = {}\n", dirfd, path, flags, mode, fd.fd);
        return fd.fd;
    }

    int Sys::fstatat64(int dirfd, Ptr pathname, Ptr statbuf, int flags) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::fstatat64(Host::FD{dirfd}, path, flags);
        if(logSyscalls_) {
            print("Sys::fstatat64(dirfd={}, path={}, statbuf={:#x}, flags={}) = {}\n",
                        dirfd, path, statbuf.address(), flags, errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
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
        if(logSyscalls_) {
            print("Sys::pselect6(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x},sigmask={:#x}) = {}\n",
                        nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), sigmask.address(), ret);
        }
        if(readfds.address() != 0) mmu_->copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(writefds.address() != 0) mmu_->copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(exceptfds.address() != 0) mmu_->copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(timeout.address() != 0) mmu_->copyToMmu(timeout, (const u8*)&ts, sizeof(timespec));
        return ret;
    }

    long Sys::set_robust_list(Ptr head, size_t len) {
        if(logSyscalls_) print("Sys::set_robust_list({:#x}, {}) = 0\n", head.address(), len);
        // maybe we can do nothing ?
        (void)head;
        (void)len;
        return 0;
    }

    long Sys::get_robust_list(int pid, Ptr64 head_ptr, Ptr64 len_ptr) {
        if(logSyscalls_) print("Sys::get_robust_list({}, {:#x}, {:#x}) = 0\n", pid, head_ptr.address(), len_ptr.address());
        (void)pid;
        (void)head_ptr;
        (void)len_ptr;
        verify(false, "implement {get,set}_robust_list");
        return 0;
    }

    int Sys::utimensat(int dirfd, Ptr pathname, Ptr times, int flags) {
        if(logSyscalls_) print("Sys::utimensat(dirfd={}, pathname={}, times={:#x}, flags={}) = -ENOTSUP\n",
                                                          dirfd, mmu_->readString(pathname), times.address(), flags);
        return -ENOTSUP;
    }

    int Sys::eventfd2(unsigned int initval, int flags) {
        Host::FD fd = Host::eventfd2(initval, flags);
        if(logSyscalls_) {
            print("Sys::eventfd2(initval={}, flags={}) = {}\n", initval, flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::epoll_create1(int flags) {
        Host::FD fd = Host::epoll_create1(flags);
        if(logSyscalls_) {
            print("Sys::epoll_create1(flags={}) = {}\n", flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::prlimit64(pid_t pid, int resource, [[maybe_unused]] Ptr new_limit, Ptr old_limit) {
        if(logSyscalls_) 
            print("Sys::prlimit64(pid={}, resource={}, new_limit={:#x}, old_limit={:#x})", pid, resource, new_limit.address(), old_limit.address());
        if(!old_limit.address()) {
            if(logSyscalls_) print(" = 0\n");
            return 0;
        }
        auto errnoOrBuffer = Host::getrlimit(pid, resource);
        if(logSyscalls_) print(" = {}\n", errnoOrBuffer.errorOr(0));
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(old_limit, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::sched_getattr(pid_t pid, Ptr attr, unsigned int size, unsigned int flags) {
        if(logSyscalls_) 
            print("Sys::sched_getattr(pid={}, attr={:#x}, size={:#x}, flags={:#x})", pid, attr.address(), size, flags);
        return -ENOTSUP;
    }

    ssize_t Sys::getrandom(Ptr buf, size_t len, int flags) {
        if(logSyscalls_) 
            print("Sys::getrandom(buf={:#x}, len={}, flags={})\n", buf.address(), len, flags);
        std::vector<u8> buffer(len);
        std::iota(buffer.begin(), buffer.end(), 0);
        mmu_->copyToMmu(buf, buffer.data(), buffer.size());
        return (ssize_t)len;
    }

    int Sys::statx(int dirfd, Ptr pathname, int flags, unsigned int mask, Ptr statxbuf) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::statx(Host::FD{dirfd}, path, flags, mask);
        if(logSyscalls_) {
            print("Sys::statx(dirfd={}, path={}, flags={}, mask={}, statxbuf={:#x}) = {}\n",
                        dirfd, path, flags, mask, statxbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statxbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::clone3(Ptr uargs, size_t size) {
        // struct clone_args {
        //     u64 flags;        /* Flags bit mask */
        //     u64 pidfd;        /* Where to store PID file descriptor
        //                         (int *) */
        //     u64 child_tid;    /* Where to store child TID,
        //                         in child's memory (pid_t *) */
        //     u64 parent_tid;   /* Where to store child TID,
        //                         in parent's memory (pid_t *) */
        //     u64 exit_signal;  /* Signal to deliver to parent on
        //                         child termination */
        //     u64 stack;        /* Pointer to lowest byte of stack */
        //     u64 stack_size;   /* Size of stack */
        //     u64 tls;          /* Location of new TLS */
        //     u64 set_tid;      /* Pointer to a pid_t array
        //                         (since Linux 5.5) */
        //     u64 set_tid_size; /* Number of elements in set_tid
        //                         (since Linux 5.5) */
        //     u64 cgroup;       /* File descriptor for target cgroup
        //                         of child (since Linux 5.7) */
        // };
        std::vector<u64> args = mmu_->readFromMmu<u64>(uargs, size / sizeof(u64));
        verify(args.size() >= 7);
        Ptr32 child_tid { args[2] };
        u64 stackAddress = args[5] + args[6];

        Thread* currentThread = scheduler_->currentThread();
        Thread* newThread = scheduler_->createThread(currentThread->descr.pid);
        newThread->data.regs = currentThread->data.regs;
        newThread->data.regs.rip() = currentThread->data.regs.rip();
        newThread->data.regs.set(R64::RAX, 0);
        newThread->data.regs.rsp() = stackAddress;
        newThread->clear_child_tid = child_tid;
        long ret = newThread->descr.tid;
        if(!!child_tid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_->write32(child_tid, (u32)ret);
        }
        if(logSyscalls_) {
            print("Sys::clone3(uargs={:#x}, size={}) = {}\n",
                        uargs.address(), size, ret);
        }
        return (int)ret;
    }

}