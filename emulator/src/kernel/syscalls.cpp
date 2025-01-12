#include "kernel/fs/fs.h"
#include "kernel/kernel.h"
#include "kernel/syscalls.h"
#include "kernel/scheduler.h"
#include "kernel/thread.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include "x64/mmu.h"
#include "x64/cpu.h"
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <numeric>
#include <sys/socket.h>

namespace kernel {

    Sys::Sys(Kernel& kernel, x64::Mmu& mmu) :
        kernel_(kernel),
        mmu_(mmu) {
    }

    template<typename... Args>
    void Sys::print(const char* format, Args... args) const {
        fmt::print("[{}:{}@{:#12x}] ", currentThread_->description().pid, currentThread_->description().tid, currentThread_->tickInfo().current());
        fmt::print(format, args...);
        [[maybe_unused]]int ret = fflush(stdout);
    }

    void Sys::syscall(Thread* thread) {
        std::scoped_lock<std::mutex> lock(mutex_);
        currentThread_ = thread;
        ScopeGuard scopeGuard([&]() {
            currentThread_ = nullptr;
        });
        x64::Registers& threadRegs = currentThread_->savedCpuState().regs;
        u64 sysNumber = threadRegs.get(x64::R64::RAX);
        currentThread_->stats().syscalls++;
        if(kernel_.isProfiling()) currentThread_->didSyscall(sysNumber);
        RegisterDump regs {{
            threadRegs.get(x64::R64::RDI),
            threadRegs.get(x64::R64::RSI),
            threadRegs.get(x64::R64::RDX),
            threadRegs.get(x64::R64::R10),
            threadRegs.get(x64::R64::R8),
            threadRegs.get(x64::R64::R9),
        }};

        switch(sysNumber) {
            case 0x0: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::read, regs));
            case 0x1: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::write, regs));
            case 0x3: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::close, regs));
            case 0x4: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::stat, regs));
            case 0x5: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::fstat, regs));
            case 0x6: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::lstat, regs));
            case 0x7: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::poll, regs));
            case 0x8: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::lseek, regs));
            case 0x9: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::mmap, regs));
            case 0xa: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::mprotect, regs));
            case 0xb: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::munmap, regs));
            case 0xc: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::brk, regs));
            case 0xd: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::rt_sigaction, regs));
            case 0xe: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::rt_sigprocmask, regs));
            case 0x10: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::ioctl, regs));
            case 0x11: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::pread64, regs));
            case 0x12: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::pwrite64, regs));
            case 0x13: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::readv, regs));
            case 0x14: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::writev, regs));
            case 0x15: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::access, regs));
            case 0x16: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::pipe, regs));
            case 0x17: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::select, regs));
            case 0x18: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::sched_yield, regs));
            case 0x19: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::mremap, regs));
            case 0x1b: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::mincore, regs));
            case 0x1c: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::madvise, regs));
            case 0x1d: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::shmget, regs));
            case 0x20: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::dup, regs));
            case 0x21: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::dup2, regs));
            case 0x26: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::setitimer, regs));
            case 0x27: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getpid, regs));
            case 0x29: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::socket, regs));
            case 0x2a: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::connect, regs));
            case 0x2c: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::sendto, regs));
            case 0x2d: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::recvfrom, regs));
            case 0x2e: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::sendmsg, regs));
            case 0x2f: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::recvmsg, regs));
            case 0x30: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::shutdown, regs));
            case 0x31: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::bind, regs));
            case 0x32: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::listen, regs));
            case 0x33: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getsockname, regs));
            case 0x34: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getpeername, regs));
            case 0x36: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::setsockopt, regs));
            case 0x37: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::getsockopt, regs));
            case 0x38: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::clone, regs));
            case 0x3c: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::exit, regs));
            case 0x3e: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::kill, regs));
            case 0x3f: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::uname, regs));
            case 0x48: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::fcntl, regs));
            case 0x49: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::flock, regs));
            case 0x4a: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::fsync, regs));
            case 0x4b: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::fdatasync, regs));
            case 0x4d: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::ftruncate, regs));
            case 0x4f: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::getcwd, regs));
            case 0x50: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::chdir, regs));
            case 0x52: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::rename, regs));
            case 0x53: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::mkdir, regs));
            case 0x57: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::unlink, regs));
            case 0x59: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::readlink, regs));
            case 0x5a: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::chmod, regs));
            case 0x5c: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::chown, regs));
            case 0x5f: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::umask, regs));
            case 0x60: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::gettimeofday, regs));
            case 0x63: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::sysinfo, regs));
            case 0x64: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::times, regs));
            case 0x66: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getuid, regs));
            case 0x68: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getgid, regs));
            case 0x6b: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::geteuid, regs));
            case 0x6c: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getegid, regs));
            case 0x6e: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getppid, regs));
            case 0x6f: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::getpgrp, regs));
            case 0x73: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::getgroups, regs));
            case 0x76: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getresuid, regs));
            case 0x78: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getresgid, regs));
            case 0x83: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::sigaltstack, regs));
            case 0x84: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::utime, regs));
            case 0x89: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::statfs, regs));
            case 0x8a: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::fstatfs, regs));
            case 0x8d: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::setpriority, regs));
            case 0x8f: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::sched_getparam, regs));
            case 0x90: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::sched_setscheduler, regs));
            case 0x91: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::sched_getscheduler, regs));
            case 0x95: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::mlock, regs));
            case 0x9d: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::prctl, regs));
            case 0x9e: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::arch_prctl, regs));
            case 0xba: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::gettid, regs));
            case 0xbf: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::getxattr, regs));
            case 0xc0: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::lgetxattr, regs));
            case 0xc9: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::time, regs));
            case 0xca: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::futex, regs));
            case 0xcb: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::sched_setaffinity, regs));
            case 0xcc: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::sched_getaffinity, regs));
            case 0xd9: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getdents64, regs));
            case 0xda: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::set_tid_address, regs));
            case 0xdd: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::posix_fadvise, regs));
            case 0xe4: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::clock_gettime, regs));
            case 0xe5: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::clock_getres, regs));
            case 0xe6: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::clock_nanosleep, regs));
            case 0xe7: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::exit_group, regs));
            case 0xe8: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::epoll_wait, regs));
            case 0xe9: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::epoll_ctl, regs));
            case 0xea: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::tgkill, regs));
            case 0xed: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::mbind, regs));
            case 0xfd: return threadRegs.set(x64::R64::RAX, invoke_syscall_0(&Sys::inotify_init, regs));
            case 0xfe: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::inotify_add_watch, regs));
            case 0x101: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::openat, regs));
            case 0x106: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::fstatat64, regs));
            case 0x10b: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::readlinkat, regs));
            case 0x10d: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::faccessat, regs));
            case 0x10e: return threadRegs.set(x64::R64::RAX, invoke_syscall_6(&Sys::pselect6, regs));
            case 0x10f: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::ppoll, regs));
            case 0x111: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::set_robust_list, regs));
            case 0x112: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::get_robust_list, regs));
            case 0x118: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::utimensat, regs));
            case 0x11d: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::fallocate, regs));
            case 0x122: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::eventfd2, regs));
            case 0x123: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::epoll_create1, regs));
            case 0x124: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::dup3, regs));
            case 0x125: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::pipe2, regs));
            case 0x126: return threadRegs.set(x64::R64::RAX, invoke_syscall_1(&Sys::inotify_init1, regs));
            case 0x12e: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::prlimit64, regs));
            case 0x13a: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::sched_setattr, regs));
            case 0x13b: return threadRegs.set(x64::R64::RAX, invoke_syscall_4(&Sys::sched_getattr, regs));
            case 0x13e: return threadRegs.set(x64::R64::RAX, invoke_syscall_3(&Sys::getrandom, regs));
            case 0x13f: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::memfd_create, regs));
            case 0x14c: return threadRegs.set(x64::R64::RAX, invoke_syscall_5(&Sys::statx, regs));
            case 0x1b3: return threadRegs.set(x64::R64::RAX, invoke_syscall_2(&Sys::clone3, regs));
            default: break;
        }
        verify(false, [&]() {
            print("Syscall {:#x} not handled\n", sysNumber);
            print("Arguments:\n");
            print("  {:#x}\n", regs.args[0]);
            print("  {:#x}\n", regs.args[1]);
            print("  {:#x}\n", regs.args[2]);
            print("  {:#x}\n", regs.args[3]);
            print("  {:#x}\n", regs.args[4]);
            print("  {:#x}\n", regs.args[5]);
        });
    }

    ssize_t Sys::read(int fd, x64::Ptr8 buf, size_t count) {
        auto errnoOrBuffer = kernel_.fs().read(FS::FD{fd}, count);
        if(logSyscalls_) {
            print("Sys::read(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::write(int fd, x64::Ptr8 buf, size_t count) {
        std::vector<u8> buffer = mmu_.readFromMmu<u8>(buf, count);
        ssize_t ret = kernel_.fs().write(FS::FD{fd}, buffer.data(), buffer.size());
        if(logSyscalls_) {
            print("Sys::write(fd={}, buf={:#x}, count={}) = {}\n",
                        fd, buf.address(), count, ret);
        }
        return ret;
    }

    int Sys::close(int fd) {
        int ret = kernel_.fs().close(FS::FD{fd});
        if(logSyscalls_) print("Sys::close(fd={}) = {}\n", fd, ret);
        return ret;
    }

    int Sys::stat(x64::Ptr pathname, x64::Ptr statbuf) {
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = kernel_.fs().stat(path);
        if(logSyscalls_) {
            print("Sys::stat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstat(int fd, x64::Ptr8 statbuf) {
        ErrnoOrBuffer errnoOrBuffer = kernel_.fs().fstat(FS::FD{fd});
        if(logSyscalls_) {
            print("Sys::fstat(fd={}, statbuf={:#x}) = {}\n",
                        fd, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::lstat(x64::Ptr pathname, x64::Ptr statbuf) {
        std::string path = mmu_.readString(pathname);
        ErrnoOrBuffer errnoOrBuffer = Host::lstat(path);
        if(logSyscalls_) {
            print("Sys::lstat(path={}, statbuf={:#x}) = {}\n",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::poll(x64::Ptr fds, size_t nfds, int timeout) {
        assert(sizeof(FS::PollData) == Host::pollRequiredBufferSize(1));
        std::vector<FS::PollData> pollfds = mmu_.readFromMmu<FS::PollData>(fds, nfds);
        if(timeout == 0) {
            auto errnoOrBufferAndReturnValue = kernel_.fs().pollImmediate(pollfds);
            if(logSyscalls_) {
                std::vector<std::string> allfds;
                for(const auto& pfd : pollfds) allfds.push_back(fmt::format("[fd={}, events={}]", pfd.fd, (int)pfd.events));
                auto fdsString = fmt::format("{}", fmt::join(allfds, ", "));
                print("Sys::poll(fds={:#x}, nfds={} (fds={}), timeout={}) = {}\n",
                            fds.address(), nfds, fdsString, timeout, errnoOrBufferAndReturnValue.errorOr(0));
                for(const auto& pd : pollfds) {
                    fmt::print("  fd={}  events={}, revents={}\n", pd.fd, (int)pd.events, (int)pd.revents);
                }
            }
            return errnoOrBufferAndReturnValue.errorOrWith<int>([&](const auto& bufferAndRetVal) {
                mmu_.copyToMmu(fds, bufferAndRetVal.buffer.data(), bufferAndRetVal.buffer.size());
                return bufferAndRetVal.returnValue;
            });
        } else {
            if(logSyscalls_) {
                std::vector<std::string> allfds;
                for(const auto& pfd : pollfds) allfds.push_back(fmt::format("[fd={}, events={}]", pfd.fd, (int)pfd.events));
                auto fdsString = fmt::format("{}", fmt::join(allfds, ", "));
                print("Sys::poll(fds={:#x}, nfds={} (fds={}), timeout={}) = pending\n",
                            fds.address(), nfds, fdsString, timeout);
            }
            kernel_.scheduler().poll(currentThread_, fds, nfds, timeout);
        }
        return 0;
    }

    off_t Sys::lseek(int fd, off_t offset, int whence) {
        off_t ret = kernel_.fs().lseek(FS::FD{fd}, offset, whence);
        if(logSyscalls_) print("Sys::lseek(fd={}, offset={:#x}, whence={}) = {}\n", fd, offset, whence, ret);
        return ret;
    }

    x64::Ptr Sys::mmap(x64::Ptr addr, size_t length, int prot, int flags, int fd, off_t offset) {
        BitFlags<x64::MAP> mmapFlags;
        if(Host::Mmap::isAnonymous(flags)) mmapFlags.add(x64::MAP::ANONYMOUS);
        if(Host::Mmap::isFixed(flags)) mmapFlags.add(x64::MAP::FIXED);
        if(Host::Mmap::isPrivate(flags)) mmapFlags.add(x64::MAP::PRIVATE);
        if(Host::Mmap::isShared(flags)) mmapFlags.add(x64::MAP::SHARED);

        BitFlags<x64::PROT> protFlags = BitFlags<x64::PROT>::fromIntegerType(prot);

        if(mmapFlags.test(x64::MAP::SHARED) && protFlags.test(x64::PROT::WRITE)) {
            warn("Writable and shared mapping not supported. Making mapping private.");
            mmapFlags.remove(x64::MAP::SHARED);
            mmapFlags.add(x64::MAP::PRIVATE);
        }

        u64 base = mmu_.mmap(addr.address(), length, protFlags, mmapFlags);
        if(!mmapFlags.test(x64::MAP::ANONYMOUS)) {
            verify(fd >= 0);
            ErrnoOrBuffer data = kernel_.fs().pread(FS::FD{fd}, length, offset);
            if(data.isError()) {
                auto filename = kernel_.fs().filename(FS::FD{fd});
                warn(fmt::format("Could not mmap file \"{}\" with fd={}", filename, fd));
                base = (u64)data.errorOr(0);
            }
            data.errorOrWith<int>([&](const Buffer& buffer) {
                BitFlags<x64::PROT> saved = mmu_.prot(base);
                BitFlags<x64::PROT> savedAndWriteable = saved;
                savedAndWriteable.add(x64::PROT::WRITE);
                mmu_.mprotect(base, length, savedAndWriteable);
                mmu_.copyToMmu(x64::Ptr8{base}, buffer.data(), buffer.size());
                mmu_.mprotect(base, length, saved);
                auto filename = kernel_.fs().filename(FS::FD{fd});
                mmu_.setRegionName(base, filename);
                return 0;
            });
        }
        if(logSyscalls_) {
            BitFlags<x64::PROT> protFlags = BitFlags<x64::PROT>::fromIntegerType(prot);
            bool protRead = protFlags.test(x64::PROT::READ);
            bool protWrite = protFlags.test(x64::PROT::WRITE);
            bool protExec = protFlags.test(x64::PROT::EXEC);
            std::string protString = fmt::format("{}{}{}",
                    protRead  ? "R" : "",
                    protWrite ? "W" : "",
                    protExec  ? "X" : "");
            std::string flagsString = fmt::format("{}{}{}{}",
                    mmapFlags.test(x64::MAP::ANONYMOUS) ? "ANONYMOUS " : "",
                    mmapFlags.test(x64::MAP::FIXED) ? "FIXED " : "",
                    mmapFlags.test(x64::MAP::PRIVATE) ? "PRIVATE " : "",
                    mmapFlags.test(x64::MAP::SHARED) ? "SHARED " : "");
            print("Sys::mmap(addr={:#x}, length={}, prot={}, flags={}, fd={}, offset={}) = {:#x}\n",
                    addr.address(), length, protString, flagsString, fd, offset, base);
        }
        return x64::Ptr{base};
    }

    int Sys::mprotect(x64::Ptr addr, size_t length, int prot) {
        BitFlags<x64::PROT> protFlags = BitFlags<x64::PROT>::fromIntegerType(prot);
        int ret = mmu_.mprotect(addr.address(), length, protFlags);
        if(logSyscalls_) {
            bool protRead = protFlags.test(x64::PROT::READ);
            bool protWrite = protFlags.test(x64::PROT::WRITE);
            bool protExec = protFlags.test(x64::PROT::EXEC);
            std::string protString = fmt::format("{}{}{}",
                    protRead  ? "R" : "",
                    protWrite ? "W" : "",
                    protExec  ? "X" : "");
            print("Sys::mprotect(addr={:#x}, length={}, prot={}) = {}\n", addr.address(), length, protString, ret);
        }
        return ret;
    }

    int Sys::munmap(x64::Ptr addr, size_t length) {
        int ret = mmu_.munmap(addr.address(), length);
        if(logSyscalls_) print("Sys::munmap(addr={:#x}, length={}) = {}\n", addr.address(), length, ret);
        return ret;
    }

    x64::Ptr Sys::brk(x64::Ptr addr) {
        u64 newBrk = mmu_.brk(addr.address());
        if(logSyscalls_) print("Sys::brk(addr={:#x}) = {:#x}\n", addr.address(), newBrk);
        return x64::Ptr{newBrk};
    }

    int Sys::rt_sigaction(int sig, x64::Ptr act, x64::Ptr oact, size_t sigsetsize) {
        if(logSyscalls_) print("Sys::rt_sigaction({}, {:#x}, {:#x}, {}) = 0\n", sig, act.address(), oact.address(), sigsetsize);
        (void)sig;
        (void)act;
        (void)oact;
        (void)sigsetsize;
        return 0;
    }

    int Sys::rt_sigprocmask(int how, x64::Ptr nset, x64::Ptr oset, size_t sigsetsize) {
        if(logSyscalls_) print("Sys::rt_sigprocmask({}, {:#x}, {:#x}, {}) = 0\n", how, nset.address(), oset.address(), sigsetsize);
        (void)how;
        (void)nset;
        (void)oset;
        (void)sigsetsize;
        return 0;
    }

    int Sys::ioctl(int fd, unsigned long request, x64::Ptr argp) {
        // We need to ask the host for the expected buffer size behind argp.
        auto bufferSize = Host::ioctlRequiredBufferSize(request);
        if(!bufferSize) {
            warn(fmt::format("Unknown ioctl {:#x}. Returning -EINVAL", request));
            return -EINVAL;
        };
        std::vector<u8> buf(bufferSize.value(), 0x0);
        Buffer buffer(std::move(buf));
        mmu_.copyFromMmu(buffer.data(), argp, buffer.size());

        auto fsrequest = [](unsigned long hostRequest) -> std::optional<Ioctl> {
            if(Host::Ioctl::isFIOCLEX(hostRequest)) return Ioctl::fioclex;
            if(Host::Ioctl::isFIONCLEX(hostRequest)) return Ioctl::fionclex;
            if(Host::Ioctl::isFIONBIO(hostRequest)) return Ioctl::fionbio;
            if(Host::Ioctl::isTCGETS(hostRequest)) return Ioctl::tcgets;
            if(Host::Ioctl::isTCSETS(hostRequest)) return Ioctl::tcsets;
            if(Host::Ioctl::isTCSETSW(hostRequest)) return Ioctl::tcsetsw;
            if(Host::Ioctl::isTIOCGWINSZ(hostRequest)) return Ioctl::tiocgwinsz;
            if(Host::Ioctl::isTIOCSWINSZ(hostRequest)) return Ioctl::tiocswinsz;
            if(Host::Ioctl::isTIOCGPGRP(hostRequest)) return Ioctl::tiocgpgrp;
            return {};
        }(request);
        verify(!!fsrequest, "Unknown request");
        auto errnoOrBuffer = kernel_.fs().ioctl(FS::FD{fd}, fsrequest.value(), buffer);
        if(logSyscalls_) {
            print("Sys::ioctl(fd={}, request={}, argp={:#x}) = {}\n",
                        fd, Host::ioctlName(request), argp.address(),
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            // The buffer returned by ioctl is empty when nothing needs to be written back.
            mmu_.copyToMmu(argp, buffer.data(), buffer.size());
            return 0;
        });
    }

    ssize_t Sys::pread64(int fd, x64::Ptr buf, size_t count, off_t offset) {
        auto errnoOrBuffer = kernel_.fs().pread(FS::FD{fd}, count, offset);
        if(logSyscalls_) {
            print("Sys::pread64(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::pwrite64(int fd, x64::Ptr buf, size_t count, off_t offset) {
        std::vector<u8> buffer = mmu_.readFromMmu<u8>(buf, count);
        auto errnoOrNbytes = kernel_.fs().pwrite(FS::FD{fd}, buffer.data(), buffer.size(), offset);
        if(logSyscalls_) {
            print("Sys::pwrite64(fd={}, buf={:#x}, count={}, offset={}) = {}\n",
                        fd, buf.address(), count, offset, errnoOrNbytes);
        }
        return errnoOrNbytes;
    }

    ssize_t Sys::readv(int fd, x64::Ptr iov, int iovcnt) {
        std::vector<u8> iovecs = mmu_.readFromMmu<u8>(iov, ((size_t)iovcnt) * Host::iovecRequiredBufferSize());
        Buffer iovecBuffer(std::move(iovecs));
        std::vector<Buffer> buffers;
        buffers.reserve((size_t)iovcnt);
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            x64::Ptr base{Host::iovecBase(iovecBuffer, i)};
            size_t len = Host::iovecLen(iovecBuffer, i);
            std::vector<u8> data(len);
            mmu_.copyFromMmu(data.data(), base, len);
            buffers.push_back(Buffer(std::move(data)));
        }
        ssize_t nbytes = kernel_.fs().readv(FS::FD{fd}, &buffers);
        if(nbytes >= 0) {
            for(size_t i = 0; i < (size_t)iovcnt; ++i) {
                x64::Ptr base{Host::iovecBase(iovecBuffer, i)};
                mmu_.copyToMmu(base, buffers[i].data(), buffers[i].size());
            }
        }
        if(logSyscalls_) print("Sys::readv(fd={}, iov={:#x}, iovcnt={}) = {}\n", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    ssize_t Sys::writev(int fd, x64::Ptr iov, int iovcnt) {
        std::vector<u8> iovecs = mmu_.readFromMmu<u8>(iov, ((size_t)iovcnt) * Host::iovecRequiredBufferSize());
        Buffer iovecBuffer(std::move(iovecs));
        std::vector<Buffer> buffers;
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            x64::Ptr base{Host::iovecBase(iovecBuffer, i)};
            size_t len = Host::iovecLen(iovecBuffer, i);
            std::vector<u8> data;
            data.resize(len);
            mmu_.copyFromMmu(data.data(), base, len);
            buffers.push_back(Buffer(std::move(data)));
        }
        ssize_t nbytes = kernel_.fs().writev(FS::FD{fd}, buffers);
        if(logSyscalls_) print("Sys::writev(fd={}, iov={:#x}, iovcnt={}) = {}\n", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    int Sys::access(x64::Ptr pathname, int mode) {
        std::string path = mmu_.readString(pathname);
        int ret = kernel_.fs().access(path, mode);
        if(logSyscalls_) {
            print("Sys::access(path={}, mode={}) = {}\n", path, mode, ret);
        }
        return ret;
    }
    
    int Sys::pipe(x64::Ptr32 pipefd) {
        auto errnoOrFds = kernel_.fs().pipe2(0);
        int ret = errnoOrFds.errorOrWith<int>([&](std::pair<FS::FD, FS::FD> fds) {
            std::vector<u32> fdsbuf {{ (u32)fds.first.fd, (u32)fds.second.fd }};
            x64::Ptr ptr { pipefd.address() };
            mmu_.writeToMmu(ptr, fdsbuf);
            return 0;
        });
        if(logSyscalls_) {
            print("Sys::pipe(pipefd={:#x}) = {}\n", pipefd.address(), ret);
        }
        return ret;
    }

    int Sys::dup(int oldfd) {
        FS::FD newfd = kernel_.fs().dup(FS::FD{oldfd});
        if(logSyscalls_) print("Sys::dup(oldfd={}) = {}\n", oldfd, newfd.fd);
        return newfd.fd;
    }

    int Sys::dup2(int oldfd, int newfd) {
        FS::FD fd = kernel_.fs().dup2(FS::FD{oldfd}, FS::FD{newfd});
        if(logSyscalls_) print("Sys::dup2(oldfd={}, newfd={}) = {}\n", oldfd, newfd, fd.fd);
        return fd.fd;
    }

    int Sys::setitimer(int which, const x64::Ptr new_value, x64::Ptr old_value) {
        if(logSyscalls_) {
            print("Sys::setitimer(which={}, new_value={:#x}, old_value={:#x}) = {}\n",
                                    which, new_value.address(), old_value.address(), -ENOTSUP);
        }
        warn(fmt::format("setitimer not implemented"));
        return -ENOTSUP;
    }

    int Sys::getpid() {
        verify(!!currentThread_);
        int pid = currentThread_->description().pid;
        if(logSyscalls_) print("Sys::getpid() = {}\n", pid);
        return pid;
    }

    int Sys::select(int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout) {
        // assert(sizeof(FS::PollData) == Host::pollRequiredBufferSize(1));
        // std::vector<FS::PollData> pollfds = mmu_.readFromMmu<FS::PollData>(fds, nfds);
        // if(timeout == 0) {
        //     auto errnoOrBufferAndReturnValue = kernel_.fs().pollImmediate(pollfds);
        //     if(logSyscalls_) {
        //         print("Sys::poll(fds={:#x}, nfds={}, timeout={}) = {}\n",
        //                     fds.address(), nfds, timeout, errnoOrBufferAndReturnValue.errorOrWith<int>([](const auto&){ return 0; }));
        //     }
        //     return errnoOrBufferAndReturnValue.errorOrWith<int>([&](const auto& bufferAndRetVal) {
        //         mmu_.copyToMmu(fds, bufferAndRetVal.buffer.data(), bufferAndRetVal.buffer.size());
        //         return bufferAndRetVal.returnValue;
        //     });
        // } else {
        //     kernel_.scheduler().poll(currentThread_, fds, nfds, timeout);
        // }
        // return 0;

        static_assert(sizeof(FS::SelectData::readfds) == sizeof(fd_set));
        FS::SelectData selectData;
        selectData.nfds = nfds;
        if(!!readfds) mmu_.copyFromMmu((u8*)&selectData.readfds, readfds, sizeof(selectData.readfds));
        if(!!writefds) mmu_.copyFromMmu((u8*)&selectData.writefds, writefds, sizeof(selectData.writefds));
        if(!!exceptfds) mmu_.copyFromMmu((u8*)&selectData.exceptfds, exceptfds, sizeof(selectData.exceptfds));
        Timer* timer = kernel_.timers().getOrTryCreate(0);
        auto timeoutDuration = timer->readTimeval(mmu_, timeout);
        if(!!timeoutDuration && timeoutDuration->seconds == 0 && timeoutDuration->nanoseconds == 0) {
            int ret = kernel_.fs().selectImmediate(&selectData);
            if(logSyscalls_) {
                print("Sys::select(nfds={}, readfds=:#x, writefds=:#x, exceptfds=:#x, timeout=:#x) = {}\n",
                            nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), ret);
            }
            if(ret < 0) return ret;
            if(!!readfds) mmu_.copyToMmu(readfds, (const u8*)&selectData.readfds, sizeof(selectData.readfds));
            if(!!writefds) mmu_.copyToMmu(writefds, (const u8*)&selectData.writefds, sizeof(selectData.writefds));
            if(!!exceptfds) mmu_.copyToMmu(exceptfds, (const u8*)&selectData.exceptfds, sizeof(selectData.exceptfds));
            return ret;
        } else {
            kernel_.scheduler().select(currentThread_, nfds, readfds, writefds, exceptfds, timeout);
            return 0;
        }
    }

    int Sys::sched_yield() {
        if(logSyscalls_) print("Sys::sched_yield()\n");
        verify(!!currentThread_);
        currentThread_->yield();
        return 0;
    }

    x64::Ptr Sys::mremap(x64::Ptr old_address, size_t old_size, size_t new_size, int flags, x64::Ptr new_address) {
        if(logSyscalls_) {
            print("Sys::mremap(old_address={:#x}, old_size={}, new_size={}, flags={}, new_address={:#x}) = {}\n",
                                    old_address.address(), old_size, new_size, flags, new_address.address(), -ENOTSUP);
        }
        warn(fmt::format("mremap not implemented"));
        return x64::Ptr{(u64)-ENOTSUP};
    }

    int Sys::mincore(x64::Ptr addr, size_t length, x64::Ptr8 vec) {
        auto res = mmu_.mincore(addr.address(), length);
        mmu_.copyToMmu(vec, res.data(), res.size());
        if(logSyscalls_) {
            print("Sys::mincore(addr={:#x}, length={:#x}, vec={:#x}) = {}\n",
                                    addr.address(), length, vec.address(), 0);
        }
        return 0;
    }

    int Sys::madvise(x64::Ptr addr, size_t length, int advice) {
        if(Host::Madvise::isDontNeed(advice)) {
            if(logSyscalls_) {
                print("Sys::madvise(addr={:#x}, length={}, advice=DONT_NEED) = {}\n",
                                        addr.address(), length, advice, 0);
            }
            return 0;
        } else {
            int ret = 0;
            if(logSyscalls_) {
                print("Sys::madvise(addr={:#x}, length={}, advice={}) = {}\n",
                                        addr.address(), length, advice, ret);
            }
            warn(fmt::format("madvise not implemented with advice {} - returning bogus 0", advice));
            return ret;
        }
    }

    int Sys::shmget(key_t key, size_t size, int shmflg) {
        if(logSyscalls_) {
            print("Sys::shmget(key={}, size={:#x}, shmflg={:#x}) = {}\n", key, size, shmflg, -ENOTSUP);
        }
        warn(fmt::format("shmget not implemented"));
        return -ENOTSUP;
    }

    int Sys::socket(int domain, int type, int protocol) {
        FS::FD fd = kernel_.fs().socket(domain, type, protocol);
        if(logSyscalls_) {
            print("Sys::socket(domain={}, type={}, protocol={}) = {}\n",
                                    domain, type, protocol, fd.fd);
        }
        return fd.fd;
    }

    int Sys::connect(int sockfd, x64::Ptr addr, size_t addrlen) {
        std::vector<u8> addrBuffer = mmu_.readFromMmu<u8>(addr, addrlen);
        Buffer buf(std::move(addrBuffer));
        int ret = kernel_.fs().connect(FS::FD{sockfd}, buf);
        if(logSyscalls_) {
            print("Sys::connect(sockfd={}, addr={:#x}, addrlen={}) = {}\n",
                        sockfd, addr.address(), addrlen, ret);
        }
        return ret;
    }

    ssize_t Sys::sendto(int sockfd, x64::Ptr buf, size_t len, int flags, x64::Ptr dest_addr, socklen_t addrlen) {
        verify(dest_addr.address() == 0);
        verify(addrlen == 0);
        std::vector<u8> bufData = mmu_.readFromMmu<u8>(buf, len);
        Buffer buffer(std::move(bufData));
        ssize_t ret = kernel_.fs().send(FS::FD{sockfd}, buffer, flags);
        if(logSyscalls_) {
            print("Sys::sendto(sockfd={}, buf={:#x}, len={}, flags={}, dest_addr={:#x}, addrlen={}) = {}\n",
                        sockfd, buf.address(), len, flags, dest_addr.address(), addrlen, ret);
        }
        return ret;
    }

    int Sys::getsockname(int sockfd, x64::Ptr addr, x64::Ptr32 addrlen) {
        u32 buffersize = mmu_.read32(addrlen);
        ErrnoOrBuffer sockname = kernel_.fs().getsockname(FS::FD{sockfd}, buffersize);
        if(logSyscalls_) {
            print("Sys::getsockname(sockfd={}, addr={:#x}, addrlen={:#x}) = {}\n",
                        sockfd, addr.address(), addrlen.address(), sockname.errorOr(0));
            // sockname.errorOrWith<int>([&](const auto& buffer) {
            //     print("{:p}\n", (const char*)buffer.data());
            //     return 0;
            // });
        }
        return sockname.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(addr, buffer.data(), buffer.size());
            mmu_.write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::getpeername(int sockfd, x64::Ptr addr, x64::Ptr32 addrlen) {
        u32 buffersize = mmu_.read32(addrlen);
        ErrnoOrBuffer peername = kernel_.fs().getpeername(FS::FD{sockfd}, buffersize);
        if(logSyscalls_) {
            print("Sys::getpeername(sockfd={}, addr={:#x}, addrlen={:#x}) = {}\n",
                        sockfd, addr.address(), addrlen.address(), peername.errorOr(0));
            // peername.errorOrWith<int>([&](const auto& buffer) {
            //     print("{}\n", (const char*)buffer.data());
            //     return 0;
            // });
        }
        return peername.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(addr, buffer.data(), buffer.size());
            mmu_.write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::setsockopt(int sockfd, int level, int optname, x64::Ptr optval, socklen_t optlen) {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        verify(!!optval, "getsockopt with null optval not implemented");
        Buffer buf(mmu_.readFromMmu<u8>(optval, (size_t)optlen));
        int ret = kernel_.fs().setsockopt(FS::FD{sockfd}, level, optname, buf);
        if(logSyscalls_) {
            print("Sys::setsockopt(sockfd={}, level={}, optname={}, optval={:#x}, optlen={}) = {}\n",
                        sockfd, level, optname, optval.address(), optlen, ret);
        }
        return ret;
    }
    
    int Sys::getsockopt(int sockfd, int level, int optname, x64::Ptr optval, x64::Ptr32 optlen) {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        verify(!!optval, "getsockopt with null optval not implemented");
        verify(!!optlen, "getsockopt with null optlen not implemented");
        u32 len = mmu_.read32(optlen);
        Buffer buf(mmu_.readFromMmu<u8>(optval, len));
        ErrnoOrBuffer errnoOrBuffer = kernel_.fs().getsockopt(FS::FD{sockfd}, level, optname, buf);
        int ret = errnoOrBuffer.errorOrWith<int>([&](const Buffer& buffer) {
            mmu_.copyToMmu(optval, buffer.data(), buffer.size());
            mmu_.write32(optlen, (u32)buffer.size());
            return 0;
        });
        if(logSyscalls_) {
            print("Sys::getsockopt(sockfd={}, level={}, optname={}, optval={:#x}, optlen={:#x}) = {}\n",
                        sockfd, level, optname, optval.address(), optlen.address(), ret);
        }
        return ret;
    }

    void checkCloneFlags(const Host::CloneFlags& flags) {
        verify(flags.childClearTid, "Expected cloneFlags.childClearTid == true");
        verify(!flags.childSetTid, "Expected cloneFlags.childSetTid == false");
        verify(!flags.clearSignalHandlers, "Expected cloneFlags.clearSignalHandlers == false");
        verify(flags.cloneSignalHandlers, "Expected cloneFlags.cloneSignalHandlers == true");
        verify(flags.cloneFiles, "Expected cloneFlags.cloneFiles == true");
        verify(flags.cloneFs, "Expected cloneFlags.cloneFs == true");
        verify(!flags.cloneIo, "Expected cloneFlags.cloneIo == false");
        verify(!flags.cloneParent, "Expected cloneFlags.cloneParent == false");
        verify(flags.parentSetTid, "Expected cloneFlags.parentSetTid == true");
        verify(!flags.clonePidFd, "Expected cloneFlags.clonePidFd == false");
        verify(flags.setTls, "Expected cloneFlags.setTls == true");
        verify(flags.cloneThread, "Expected cloneFlags.cloneThread == true");
        verify(flags.cloneVm, "Expected cloneFlags.cloneVm) == true");
        verify(!flags.cloneVfork, "Expected cloneFlags.cloneVfork) == false");
    }

    long Sys::clone(unsigned long flags, x64::Ptr stack, x64::Ptr32 parent_tid, x64::Ptr32 child_tid, unsigned long tls) {
        verify(!!currentThread_);
        std::unique_ptr<Thread> newThread = kernel_.scheduler().allocateThread(currentThread_->description().pid);
        verify(!!newThread);
        const Thread::SavedCpuState& oldCpuState = currentThread_->savedCpuState();
        Thread::SavedCpuState& newCpuState = newThread->savedCpuState();
        newCpuState.regs = oldCpuState.regs;
        newCpuState.regs.set(x64::R64::RAX, 0);
        newCpuState.regs.rip() = oldCpuState.regs.rip();
        newCpuState.regs.rsp() = stack.address();
        mmu_.setRegionName(stack.address(), fmt::format("Stack of thread {}", newThread->description().tid));
        newCpuState.fsBase = tls;
        long ret = newThread->description().tid;

        Host::CloneFlags cloneFlags = Host::fromCloneFlags(flags);
        checkCloneFlags(cloneFlags);

        if(cloneFlags.childClearTid) {
            newThread->setClearChildTid(child_tid);
        }
        if(!!child_tid && cloneFlags.childSetTid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_.write32(child_tid, (u32)ret);
        }
        if(!!parent_tid && cloneFlags.parentSetTid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_.write32(parent_tid, (u32)ret);
        }
        if(logSyscalls_) {
            print("Sys::clone(flags={}, stack={:#x}, parent_tid={:#x}, child_tid={:#x}, tls={}) = {}\n",
                        flags, stack.address(), parent_tid.address(), child_tid.address(), tls, ret);
        }
        kernel_.scheduler().addThread(std::move(newThread));
        return ret;
    }

    int Sys::exit(int status) {
        if(logSyscalls_) {
            print("Sys::exit(status={})\n", status);
        }
        kernel_.scheduler().terminate(currentThread_, status);
        return status;
    }

    int Sys::kill(pid_t pid, int sig) {
        if(logSyscalls_) {
            print("Sys::kill(pid={}, sig={}) = {}\n", pid, sig, -ENOTSUP);
        }
        warn(fmt::format("kill not implemented"));
        return -ENOTSUP;
    }

    int Sys::uname(x64::Ptr buf) {
        ErrnoOrBuffer errnoOrBuffer = Host::uname();
        if(logSyscalls_) {
            print("Sys::uname(buf={:#x}) = {}\n",
                        buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fcntl(int fd, int cmd, int arg) {
        int ret = kernel_.fs().fcntl(FS::FD{fd}, cmd, arg);
        if(logSyscalls_) print("Sys::fcntl(fd={}, cmd={}, arg={}) = {}\n", fd, Host::fcntlName(cmd), arg, ret);
        return ret;
    }

    int Sys::flock(int fd, int operation) {
        int ret = kernel_.fs().flock(FS::FD{fd}, operation);
        if(logSyscalls_) print("Sys::flock(fd={}, operation={}) = {}\n", fd, operation, ret);
        return ret;
    }

    int Sys::fsync(int fd) {
        if(logSyscalls_) print("Sys::fsync(fd={}) = {}\n", fd, -ENOTSUP);
        warn(fmt::format("fsync not implemented"));
        return -ENOTSUP;
    }

    int Sys::fdatasync(int fd) {
        if(logSyscalls_) print("Sys::fdatasync(fd={}) = {}\n", fd, -ENOTSUP);
        warn(fmt::format("fdatasync not implemented"));
        return -ENOTSUP;
    }

    int Sys::ftruncate(int fd, off_t length) {
        int ret = kernel_.fs().ftruncate(FS::FD{fd}, length);
        if(logSyscalls_) print("Sys::ftruncate(fd={}, length={}) = {}\n", fd, length, ret);
        return ret;
    }

    int Sys::getcwd(x64::Ptr buf, size_t size) {
        ErrnoOrBuffer errnoOrBuffer = Host::getcwd(size);
        if(logSyscalls_) {
            print("Sys::getcwd(buf={:#x}, size={}) = {:#x}\n",
                        buf.address(), size, errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
                            return (int)buffer.size();
                        }));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return (int)buffer.size();
        });
    }

    int Sys::chdir(x64::Ptr pathname) {
        auto path = mmu_.readString(pathname);
        int ret = Host::chdir(path);
        if(logSyscalls_) {
            print("Sys::chdir(path={}) = {}\n", path, ret);
        }
        warn(fmt::format("chdir not implemented"));
        return ret;
    }

    int Sys::rename(x64::Ptr oldpath, x64::Ptr newpath) {
        auto oldname = mmu_.readString(oldpath);
        auto newname = mmu_.readString(newpath);
        int ret = kernel_.fs().rename(oldname, newname);
        if(logSyscalls_) {
            print("Sys::rename(oldpath={}, newpath={}) = {}\n", oldname, newname, ret);
        }
        return ret;
    }

    int Sys::mkdir(x64::Ptr pathname, mode_t mode) {
        auto path = mmu_.readString(pathname);
        auto ret = kernel_.fs().mkdir(path);
        if(logSyscalls_) {
            print("Sys::mkdir(path={}, mode={:o}) = {}\n", path, mode, ret);
        }
        return ret;
    }

    int Sys::unlink([[maybe_unused]] x64::Ptr pathname) {
        auto path = mmu_.readString(pathname);
        int ret = kernel_.fs().unlink(path);
        if(logSyscalls_) {
            print("Sys::unlink(path={}) = {}\n", path, ret);
        }
        return ret;
    }

    ssize_t Sys::readlink(x64::Ptr pathname, x64::Ptr buf, size_t bufsiz) {
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = Host::readlink(path, bufsiz);
        if(logSyscalls_) {
            print("Sys::readlink(path={}, buf={:#x}, size={}) = {:#x}\n",
                        path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::chmod(x64::Ptr pathname, mode_t mode) {
        if(logSyscalls_) {
            std::string path = mmu_.readString(pathname);
            print("Sys::chmod(path={}, mode={}) = {}\n",
                        path, mode, -ENOTSUP);
        }
        warn("chmod not implemented");
        return -ENOTSUP;
    }

    int Sys::chown(x64::Ptr pathname, uid_t owner, gid_t group) {
        if(logSyscalls_) {
            std::string path = mmu_.readString(pathname);
            print("Sys::chown(path={}, owner={}, group={}) = {}\n",
                        path, owner, group, -ENOTSUP);
        }
        warn("chown not implemented");
        return -ENOTSUP;
    }

    int Sys::umask(int mask) {
        if(logSyscalls_) {
            print("Sys::umask(mask={}) = {}\n",
                        mask, 0777);
        }
        warn("umask not implemented");
        return 0777;
    }

    int Sys::gettimeofday(x64::Ptr tv, x64::Ptr tz) {
        PreciseTime time = kernel_.scheduler().kernelTime();
        auto timevalBuffer = Host::gettimeofday(time);
        auto timezoneBuffer = Host::gettimezone();
        if(logSyscalls_) {
            print("Sys::gettimeofday(tv={:#x}, tz={:#x}) = {:#x}\n",
                        tv.address(), tz.address(), 0);
        }
        if(!!tv) mmu_.copyToMmu(tv, timevalBuffer.data(), timevalBuffer.size());
        if(!!tz) mmu_.copyToMmu(tv, timezoneBuffer.data(), timezoneBuffer.size());
        return 0;
    }

    int Sys::sysinfo(x64::Ptr info) {
        auto errnoOrBuffer = Host::sysinfo();
        if(logSyscalls_) {
            print("Sys::sysinfo(info={:#x}) = {}\n",
                        info.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(info, buffer.data(), buffer.size());
            return 0;
        });
    }

    clock_t Sys::times(x64::Ptr buf) {
        if(logSyscalls_) {
            print("Sys::times(buf={:#x}) = {}\n",
                        buf.address(), -ENOTSUP);
        }
        warn("times not implemented");
        return -ENOTSUP;
    }

    int Sys::getuid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getuid();
    }

    int Sys::getgid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getgid();
    }

    int Sys::geteuid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::geteuid();
    }

    int Sys::getegid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getegid();
    }

    int Sys::getppid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getppid();
    }

    int Sys::getpgrp() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getpgrp();
    }

    int Sys::getgroups(int size, x64::Ptr list) {
        ErrnoOrBuffer groups = Host::getgroups(size);
        int ret = groups.errorOrWith<int>([&](const Buffer& buf) {
            if(size > 0) {
                mmu_.copyToMmu(list, buf.data(), buf.size());
            }
            return (int)(buf.size() / sizeof(gid_t));
        });
        if(logSyscalls_) {
            print("Sys::getgroups(size={}, list={:#x}) = {}\n", size, list.address(), ret);
        }
        return ret;
    }

    int Sys::getresuid(x64::Ptr32 ruid, x64::Ptr32 euid, x64::Ptr32 suid) {
        Host::UserCredentials creds = Host::getUserCredentials();
        mmu_.write32(ruid, (u32)creds.ruid);
        mmu_.write32(euid, (u32)creds.euid);
        mmu_.write32(suid, (u32)creds.suid);
        return 0;
    }

    int Sys::getresgid(x64::Ptr32 rgid, x64::Ptr32 egid, x64::Ptr32 sgid) {
        Host::UserCredentials creds = Host::getUserCredentials();
        mmu_.write32(rgid, (u32)creds.rgid);
        mmu_.write32(egid, (u32)creds.egid);
        mmu_.write32(sgid, (u32)creds.sgid);
        return 0;
    }

    int Sys::sigaltstack(x64::Ptr ss, x64::Ptr old_ss) {
        if(logSyscalls_) {
            print("Sys::sigaltstack(ss={:#x}, old_ss={:#x} = {})\n", ss.address(), old_ss.address(), -ENOTSUP);
        }
        warn("sigaltstack not implemented");
        return -ENOTSUP;
    }

    int Sys::utime(x64::Ptr filename, x64::Ptr times) {
        if(logSyscalls_) {
            std::string path = mmu_.readString(filename);
            print("Sys::utime(filename={}, times={:#x} = {})\n", path, times.address(), -ENOTSUP);
        }
        warn("utime not implemented");
        return -ENOTSUP;
    }

    int Sys::statfs(x64::Ptr pathname, x64::Ptr buf) {
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = Host::statfs(path);
        if(logSyscalls_) {
            print("Sys::statfs(pathname={}, buf={:#x} = {})\n", path, buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstatfs(int fd, x64::Ptr buf) {
        auto errnoOrBuffer = kernel_.fs().fstatfs(FS::FD{fd});
        if(logSyscalls_) {
            print("Sys::fstatfs(fd={}, buf={:#x} = {})\n", fd, buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::setpriority(int which, id_t who, int prio) {
        if(logSyscalls_) {
            print("Sys::setpriority(which={}, who={}, prio={}) = {}\n", which, who, prio, -ENOTSUP);
        }
        warn(fmt::format("setpriority not implemented"));
        return -ENOTSUP;
    }

    int Sys::sched_getparam(pid_t pid, x64::Ptr param) {
        if(logSyscalls_) {
            print("Sys::sched_getparam(pid={}, param={:#x}) = {}\n", pid, param.address(), -ENOTSUP);
        }
        warn(fmt::format("sched_getparam not implemented"));
        return -ENOTSUP;
    }

    int Sys::sched_setscheduler(pid_t pid, int policy, x64::Ptr param) {
        if(logSyscalls_) {
            print("Sys::sched_setscheduler(pid={}, policy={}, param={:#x}) = {}\n", pid, policy, param.address(), -ENOTSUP);
        }
        warn(fmt::format("sched_setscheduler not implemented"));
        return -ENOTSUP;
    }

    int Sys::sched_getscheduler(pid_t pid) {
        if(logSyscalls_) {
            print("Sys::sched_getscheduler(pid={}) = {}\n", pid, -ENOTSUP);
        }
        warn(fmt::format("sched_getscheduler not implemented"));
        return -ENOTSUP;
    }

    int Sys::mlock(x64::Ptr addr, size_t len) {
        if(logSyscalls_) {
            print("Sys::mlock(addr={:#x}, len={}) = {}\n", addr.address(), len, 0);
        }
        return 0;
    }

    u64 Sys::exit_group(int status) {
        if(logSyscalls_) print("Sys::exit_group(status={})\n", status);
        kernel_.scheduler().terminateAll(status);
        return (u64)status;
    }

    struct [[gnu::packed]] EpollEvent {
        u32 event;
        u64 data;
    };

    int Sys::epoll_wait(int epfd, x64::Ptr events, int maxevents, int timeout) {
        if(!events) return -EFAULT;
        if(maxevents <= 0) return -EINVAL;
        if(timeout == 0) {
            std::vector<FS::EpollEvent> epollEvents;
            int ret = kernel_.fs().epollWaitImmediate(FS::FD{epfd}, &epollEvents);
            if(ret >= 0) {
                epollEvents.resize(std::min((size_t)maxevents, epollEvents.size()));
                ret = (int)epollEvents.size();
                std::vector<EpollEvent> eventsForMemory;
                eventsForMemory.reserve(epollEvents.size());
                for(const auto& e : epollEvents) {
                    eventsForMemory.push_back(EpollEvent {
                        e.events.toUnderlying(),
                        e.data,
                    });
                }
                mmu_.writeToMmu<EpollEvent>(events, eventsForMemory);
            }
            if(logSyscalls_) {
                print("Sys::epoll_wait(epfd={}, events={:#x}, maxevents={}, timeout={})\n", epfd, events.address(), maxevents, timeout, ret);
            }
            return ret;
        } else {
            // int ret = kernel_.fs().epoll_wait(FS::FD{epfd}, events, maxevents, timeout);
            kernel_.scheduler().epoll_wait(currentThread_, epfd, events, maxevents, timeout);
            if(logSyscalls_) {
                print("Sys::epoll_wait(epfd={}, events={:#x}, maxevents={}, timeout={}) = pending\n", epfd, events.address(), maxevents, timeout);
            }
            return 0;
        }
    }

    int Sys::epoll_ctl(int epfd, int op, int fd, x64::Ptr event) {
        verify(!!event, "Null event in epoll_ctl not supported");
        EpollEvent ee = mmu_.readFromMmu<EpollEvent>(event);
        int ret = kernel_.fs().epoll_ctl(FS::FD{epfd}, op, FS::FD{fd}, BitFlags<FS::EpollEventType>::fromIntegerType(ee.event), ee.data);
        if(logSyscalls_) {
            print("Sys::epoll_ctl(epfd={}, op={}, fd={}, event=[event={:#x}, data={}]) = {}\n", epfd, op, fd, ee.event, ee.data, ret);
        }
        return ret;
    }

    int Sys::tgkill(int tgid, int tid, int sig) {
        if(logSyscalls_) print("Sys::tgkill(tgid={}, tid={}, sig={})\n", tgid, tid, sig);
        kernel_.scheduler().kill(sig);
        return 0;
    }

    int Sys::mbind(unsigned long start, unsigned long len, unsigned long mode, x64::Ptr64 nmask, unsigned long maxnode, unsigned flags) {
        if(logSyscalls_) {
            print("Sys::mbind(start={}, len={}, mode={}, nmask={:#x}, maxnode={}, flags={})\n", start, len, mode, nmask.address(), maxnode, flags);
        }
        warn(fmt::format("mbind not implemented"));
        return -ENOTSUP;
    }

    int Sys::inotify_init() {
        if(logSyscalls_) print("Sys::inotify_init() = {}\n", -ENOTSUP);
        warn(fmt::format("inotify_init not implemented"));
        return -ENOTSUP;
    }

    int Sys::inotify_add_watch(int fd, x64::Ptr pathname, uint32_t mask) {
        if(logSyscalls_) print("Sys::inotify_add_watch(fd={}, pathname={}, mask={}) = {}\n", fd, mmu_.readString(pathname), mask, -ENOTSUP);
        warn(fmt::format("inotify_add_watch not implemented"));
        return -ENOTSUP;
    }

    ssize_t Sys::getxattr(x64::Ptr path, x64::Ptr name, x64::Ptr value, size_t size) {
        auto spath = mmu_.readString(path);
        auto sname = mmu_.readString(name);
        auto errnoOrBuffer = Host::getxattr(spath, sname, size);
        if(logSyscalls_) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::lgetxattr(x64::Ptr path, x64::Ptr name, x64::Ptr value, size_t size) {
        auto spath = mmu_.readString(path);
        auto sname = mmu_.readString(name);
        auto errnoOrBuffer = Host::lgetxattr(spath, sname, size);
        if(logSyscalls_) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}\n",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    time_t Sys::time(x64::Ptr tloc) {
        time_t t = (time_t)kernel_.scheduler().kernelTime().seconds;
        if(logSyscalls_) print("Sys::time({:#x}) = {}\n", tloc.address(), t);
        if(tloc.address()) mmu_.copyToMmu(tloc, (const u8*)&t, sizeof(t));
        return t;
    }

    long Sys::futex(x64::Ptr32 uaddr, int futex_op, uint32_t val, x64::Ptr timeout, x64::Ptr32 uaddr2, uint32_t val3) {
        auto onExit = [&](long ret) -> long {
            if(!logSyscalls_) return ret;
            std::string op;
            switch(futex_op & 0x7f) {
                case 0: op = "wait"; break;
                case 1: op = "wake"; break;
                case 9: op = "wait_bitset"; break;
                default: op = fmt::format("unknown futex {}", futex_op); break;
            }
            print("Sys::futex(uaddr={:#x}, op={}, val={}, timeout={:#x}, uaddr2={:#x}, val3={}) = {}\n",
                              uaddr.address(), op, val, timeout.address(), uaddr2.address(), val3, ret);
            return ret;
        };
        int unmaskedOp = futex_op & 0x7f;
        if(unmaskedOp == 0) {
            // wait
            u32 loaded = mmu_.read32(uaddr);
            if(loaded != val) return -EAGAIN;
            // create timer 0
            Timer* timer = kernel_.timers().getOrTryCreate(0);
            verify(!!timer);
            timer->update(kernel_.scheduler().kernelTime());
            kernel_.scheduler().wait(currentThread_, uaddr, val, timeout);
            return onExit(0);
        }
        if(unmaskedOp == 1) {
            // wake
            u32 nbWoken = kernel_.scheduler().wake(uaddr, val);
            return onExit(nbWoken);
        }
        if(unmaskedOp == 5) {
            // wake_op
            u32 val2 = (u32)timeout.address();
            u32 nbWoken = kernel_.scheduler().wakeOp(uaddr, val, uaddr2, val2, val3);
            return onExit(nbWoken);
        }
        if(unmaskedOp == 7) {
            warn("futex_unlock_pi returns bogus ENOSYS value");
            return onExit(-ENOSYS);
        }
        if(unmaskedOp == 9 && val3 == std::numeric_limits<uint32_t>::max()) {
            // wait_bitset
            u32 loaded = mmu_.read32(uaddr);
            if(loaded != val) return -EAGAIN;
            // create timer 0
            Timer* timer = kernel_.timers().getOrTryCreate(0);
            verify(!!timer);
            timer->update(kernel_.scheduler().kernelTime());
            kernel_.scheduler().waitBitset(currentThread_, uaddr, val, timeout);
            return onExit(0);
        }
        verify(false, [&]() {
            fmt::print("futex with op={} is not supported\n", unmaskedOp);
        });
        return 1;
    }

    int Sys::sched_setaffinity(pid_t pid, size_t cpusetsize, x64::Ptr mask) {
        if(logSyscalls_) {
            print("Sys::sched_setaffinity(pid={}, cpusetsize={}, mask={:#x}) = {}\n", pid, cpusetsize, mask.address(), -ENOTSUP);
        }
        warn(fmt::format("sched_setaffinity not implemented"));
        return -ENOTSUP;
    }

    int Sys::sched_getaffinity(pid_t pid, size_t cpusetsize, x64::Ptr mask) {
        int ret = 0;
        if(pid == 0) {
            // pretend that only cpu 0 is available.
            std::vector<u8> buffer;
            buffer.resize(cpusetsize, 0x0);
            if(!buffer.empty()) {
                buffer[0] |= 0x1;
            }
            mmu_.copyToMmu(mask, buffer.data(), buffer.size());
            ret = 1;
        } else {
            // don't allow looking at other processes
            ret = -EPERM;
        }
        if(logSyscalls_) print("Sys::sched_getaffinity({}, {}, {:#x}) = {}\n",
                pid, cpusetsize, mask.address(), ret);
        return ret;
    }

    ssize_t Sys::recvfrom(int sockfd, x64::Ptr buf, size_t len, int flags, x64::Ptr src_addr, x64::Ptr32 addrlen) {
        bool requireSrcAddress = !!src_addr && !!addrlen;
        ErrnoOr<std::pair<Buffer, Buffer>> ret = kernel_.fs().recvfrom(FS::FD{sockfd}, len, flags, requireSrcAddress);
        if(logSyscalls_) {
            print("Sys::recvfrom(sockfd={}, buf={:#x}, len={}, flags={}, src_addr={:#x}, addrlen={:#x}) = {}\n",
                                      sockfd, buf.address(), len, flags, src_addr.address(), addrlen.address(),
                                      ret.errorOrWith<ssize_t>([](const auto& buffers) {
                return (ssize_t)buffers.first.size();
            }));
        }
        return ret.errorOrWith<ssize_t>([&](const auto& buffers) {
            mmu_.copyToMmu(buf, buffers.first.data(), buffers.first.size());
            if(requireSrcAddress) {
                mmu_.copyToMmu(src_addr, buffers.second.data(), buffers.second.size());
                mmu_.write32(addrlen, (u32)buffers.second.size());
            }
            return (ssize_t)buffers.first.size();
        });
    }

    ssize_t Sys::sendmsg(int sockfd, x64::Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header = mmu_.readFromMmu<msghdr>(msg);

        FS::Message message;

        // read Message::msg_name
        if(!!header.msg_name && header.msg_namelen > 0) {
            std::vector<u8> msg_name_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)header.msg_name}, header.msg_namelen);
            message.msg_name = Buffer(std::move(msg_name_buffer));
        }

        // read Message::msg_iov
        std::vector<iovec> msg_iovecs = mmu_.readFromMmu<iovec>(x64::Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            std::vector<u8> msg_iovec_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iovecs[i].iov_len);
            message.msg_iov.push_back(Buffer(std::move(msg_iovec_buffer)));
        }

        // read Message::control
        if(!!header.msg_control && header.msg_controllen > 0) {
            std::vector<u8> msg_control_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)header.msg_control}, header.msg_controllen);
            message.msg_control = Buffer(std::move(msg_control_buffer));
        }

        // read Message::msg_flags
        message.msg_flags = header.msg_flags;

        ssize_t nbytes = kernel_.fs().sendmsg(FS::FD{sockfd}, flags, message);
        if(logSyscalls_) {
            print("Sys::sendmsg(sockfd={}, msg={:#x}, flags={}) = {}\n",
                        sockfd, msg.address(), flags, nbytes);
        }
        return nbytes;
    }

    ssize_t Sys::recvmsg(int sockfd, x64::Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header = mmu_.readFromMmu<msghdr>(msg);

        FS::Message message;

        // read Message::msg_name
        if(!!header.msg_name && header.msg_namelen > 0) {
            std::vector<u8> msg_name_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)header.msg_name}, header.msg_namelen);
            message.msg_name = Buffer(std::move(msg_name_buffer));
        }

        // read Message::msg_iov
        std::vector<iovec> msg_iovecs = mmu_.readFromMmu<iovec>(x64::Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            std::vector<u8> msg_iovec_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iovecs[i].iov_len);
            message.msg_iov.push_back(Buffer(std::move(msg_iovec_buffer)));
        }

        // read Message::control
        if(!!header.msg_control && header.msg_controllen > 0) {
            std::vector<u8> msg_control_buffer = mmu_.readFromMmu<u8>(x64::Ptr8{(u64)header.msg_control}, header.msg_controllen);
            message.msg_control = Buffer(std::move(msg_control_buffer));
        }

        // read Message::msg_flags
        message.msg_flags = header.msg_flags;

        // do the syscall
        ssize_t nbytes = kernel_.fs().recvmsg(FS::FD{sockfd}, flags, &message);

        // write back to header
        header.msg_namelen = (socklen_t)message.msg_name.size();
        if(!!header.msg_name) {
            mmu_.copyToMmu(x64::Ptr8{(u64)header.msg_name}, message.msg_name.data(), message.msg_name.size());
        }
        header.msg_iovlen = message.msg_iov.size();
        verify(header.msg_iovlen == message.msg_iov.size(), "message iov changed length...");
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            mmu_.copyToMmu(x64::Ptr8{(u64)msg_iovecs[i].iov_base}, message.msg_iov[i].data(), message.msg_iov[i].size());
        }
        header.msg_controllen = message.msg_control.size();
        if(!!header.msg_control) {
            mmu_.copyToMmu(x64::Ptr8{(u64)header.msg_control}, message.msg_control.data(), message.msg_control.size());
        }
        header.msg_flags = message.msg_flags;

        mmu_.writeToMmu<msghdr>(msg, header);
        
        if(logSyscalls_) {
            std::vector<std::string> iovStringElements;
            iovStringElements.reserve(message.msg_iov.size());
            for(const auto& buf : message.msg_iov) {
                iovStringElements.push_back(fmt::format("len={}", buf.size()));
            }
            std::string iovString = fmt::format("[{}]", fmt::join(iovStringElements, ", "));
            std::string messageString = fmt::format("namelen={}, name={}, "
                    "iovlen={}, iov=[{}], "
                    "controllen={}, control={}, "
                    "msg_flags={:#x}",
                        header.msg_namelen, header.msg_name,
                        header.msg_iovlen, iovString,
                        header.msg_controllen, header.msg_control,
                        header.msg_flags);
            print("Sys::recvmsg(sockfd={}, msg=[{}], flags={:#x}) = {}\n",
                        sockfd, messageString, flags, nbytes);
        }
        return nbytes;
    }

    int Sys::shutdown(int sockfd, int how) {
        int rc = kernel_.fs().shutdown(FS::FD{sockfd}, how);
        if(logSyscalls_) {
            print("Sys::shutdown(sockfd={}, how={}) = {}\n",
                        sockfd, how, rc);
        }
        return rc;
    }

    int Sys::bind(int sockfd, x64::Ptr addr, socklen_t addrlen) {
        Buffer saddr(mmu_.readFromMmu<u8>(addr, addrlen));
        int rc = kernel_.fs().bind(FS::FD{sockfd}, saddr);
        if(logSyscalls_) {
            print("Sys::bind(sockfd={}, addr={:#x}, addrlen={}) = {}\n",
                        sockfd, addr.address(), addrlen, rc);
        }
        return rc;
    }

    int Sys::listen(int sockfd, int backlog) {
        if(logSyscalls_) {
            print("Sys::listen(sockfd={}, backlog={}) = {}\n", sockfd, backlog, -ENOTSUP);
        }
        warn(fmt::format("listen not implemented"));
        return -ENOTSUP;
    }

    ssize_t Sys::getdents64(int fd, x64::Ptr dirp, size_t count) {
        auto errnoOrBuffer = kernel_.fs().getdents64(FS::FD{fd}, count);
        if(logSyscalls_) {
            print("Sys::getdents64(fd={}, dirp={:#x}, count={}) = {}\n",
                        fd, dirp.address(), count, errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(dirp, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    pid_t Sys::set_tid_address(x64::Ptr32 ptr) {
        if(logSyscalls_) print("Sys::set_tid_address({:#x}) = {}\n", ptr.address(), currentThread_->description().tid);
        currentThread_->setClearChildTid(ptr);
        return currentThread_->description().tid;
    }

    int Sys::posix_fadvise(int fd, off_t offset, off_t len, int advice) {
        if(logSyscalls_) {
            print("Sys::posix_fadvise(fd={}, offset={}, len={}, advise={}) = {}\n",
                                    fd, offset, len, advice, 0);
        }
        warn(fmt::format("posix_fadvise not implemented - returning bogus 0"));
        return 0;
    }

    int Sys::clock_gettime(clockid_t clockid, x64::Ptr tp) {
        // create the timer for future reference
        auto* timer = kernel_.timers().getOrTryCreate(clockid);
        if(!timer) return -EINVAL;
        // TODO: we should read from the timer, not the scheduler
        PreciseTime time = kernel_.scheduler().kernelTime();
        timer->update(time); // just in case
        Buffer buffer = Host::clock_gettime(time);
        mmu_.copyToMmu(tp, buffer.data(), buffer.size());
        if(logSyscalls_) {
            print("Sys::clock_gettime({}, {:#x}) = {}\n",
                        clockid, tp.address(), 0);
        }
        return 0;
    }

    int Sys::clock_getres(clockid_t clockid, x64::Ptr res) {
        auto buffer = Host::clock_getres();
        if(logSyscalls_) {
            print("Sys::clock_getres({}, {:#x}) = {}\n",
                        clockid, res.address(), 0);
        }
        mmu_.copyToMmu(res, buffer.data(), buffer.size());
        return 0;
    }

    int Sys::clock_nanosleep(clockid_t clockid, int flags, x64::Ptr request, x64::Ptr remain) {
        verify(flags == 0, "clock_nanosleep with nonzero flags not supported (relative only)");
        Timer* timer = kernel_.timers().getOrTryCreate(clockid);
        if(!timer) { return -EINVAL; }
        auto timediff = timer->readRelativeTimespec(mmu_, request);
        if(!timediff) { return -EFAULT; }
        timer->update(kernel_.scheduler().kernelTime());
        kernel_.scheduler().sleep(currentThread_, timer, timer->now() + timediff.value());
        if(logSyscalls_) {
            print("Sys::clock_nanosleep(clockid={}, flags={}, request={}s{}ns, remain={:#x}) = {}\n",
                        clockid, flags, timediff->seconds, timediff->nanoseconds, remain.address(), 0);
        }
        return 0;
    }

    int Sys::prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
        if(logSyscalls_) {
            print("Sys::prctl(option={}, arg2={}, arg3={}, arg4={}, arg5={}) = {}\n", option, arg2, arg3, arg4, arg5, -ENOTSUP);
        }
        warn(fmt::format("prctl not implemented"));
        return -ENOTSUP;
    }

    int Sys::arch_prctl(int code, x64::Ptr addr) {
        bool isSetFS = Host::Prctl::isSetFS(code);
        if(logSyscalls_) print("Sys::arch_prctl(code={}, addr={:#x}) = {}\n", code, addr.address(), isSetFS ? 0 : -EINVAL);
        if(!isSetFS) return -EINVAL;
        verify(!!currentThread_);
        currentThread_->savedCpuState().fsBase = addr.address();
        return 0;
    }

    int Sys::gettid() {
        verify(!!currentThread_);
        int tid = currentThread_->description().tid;
        if(logSyscalls_) print("Sys::gettid() = {}\n", tid);
        return tid;
    }

    int Sys::openat(int dirfd, x64::Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_.readString(pathname);
        BitFlags<FS::AccessMode> accessMode = FS::toAccessMode(flags);
        BitFlags<FS::CreationFlags> creationFlags = FS::toCreationFlags(flags);
        BitFlags<FS::StatusFlags> statusFlags = FS::toStatusFlags(flags);
        FS::Permissions permissions = FS::fromMode(mode);
        FS::FD fd = kernel_.fs().open(FS::FD{dirfd}, path, accessMode, creationFlags, statusFlags, permissions);
        if(logSyscalls_) {
            std::string flagsString = fmt::format("[{}{}{}{}{}{}{}]",
                accessMode.test(FS::AccessMode::READ)  ? "Read " : "",
                accessMode.test(FS::AccessMode::WRITE) ? "Write " : "",
                statusFlags.test(FS::StatusFlags::APPEND) ? "Append " : "",
                creationFlags.test(FS::CreationFlags::TRUNC) ? "Truncate " : "",
                creationFlags.test(FS::CreationFlags::CREAT) ? "Create " : "",
                creationFlags.test(FS::CreationFlags::CLOEXEC) ? "CloseOnExec " : "",
                creationFlags.test(FS::CreationFlags::DIRECTORY) ? "Directory " : "");
            print("Sys::openat(dirfd={}, path={}, flags={}, mode={:o}) = {}\n", dirfd, path, flagsString, mode, fd.fd);
        }
        return fd.fd;
    }

    int Sys::fstatat64(int dirfd, x64::Ptr pathname, x64::Ptr statbuf, int flags) {
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = kernel_.fs().fstatat64(FS::FD{dirfd}, path, flags);
        if(logSyscalls_) {
            print("Sys::fstatat64(dirfd={}, path={}, statbuf={:#x}, flags={}) = {}\n",
                        dirfd, path, statbuf.address(), flags, errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    ssize_t Sys::readlinkat(int dirfd, x64::Ptr pathname, x64::Ptr buf, size_t bufsiz) {
        verify(dirfd == Host::cwdfd().fd, "dirfd is not cwd");
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = Host::readlink(path, bufsiz);
        if(logSyscalls_) {
            print("Sys::readlinkat(dirfd={}, path={}, buf={:#x}, size={}) = {:#x}\n",
                        dirfd, path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_.copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::faccessat(int dirfd, x64::Ptr pathname, int mode) {
        std::string path = mmu_.readString(pathname);
        int ret = kernel_.fs().faccessat(FS::FD{dirfd}, path, mode);
        if(logSyscalls_) {
            print("Sys::faccessat(dirfd={}, path={}, mode={}) = {}\n", dirfd, path, mode, ret);
        }
        return ret;
    }

    int Sys::pselect6(int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout, x64::Ptr sigmask) {
        fd_set rfds;
        if(readfds.address() != 0) mmu_.copyFromMmu((u8*)&rfds, readfds, sizeof(fd_set));
        fd_set wfds;
        if(writefds.address() != 0) mmu_.copyFromMmu((u8*)&wfds, writefds, sizeof(fd_set));
        fd_set efds;
        if(exceptfds.address() != 0) mmu_.copyFromMmu((u8*)&efds, exceptfds, sizeof(fd_set));
        timespec ts;
        if(timeout.address() != 0) mmu_.copyFromMmu((u8*)&ts, timeout, sizeof(timespec));
        sigset_t smask;
        if(sigmask.address() != 0) mmu_.copyFromMmu((u8*)&smask, sigmask, sizeof(sigset_t));
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
        if(readfds.address() != 0) mmu_.copyToMmu(readfds, (const u8*)&rfds, sizeof(fd_set));
        if(writefds.address() != 0) mmu_.copyToMmu(writefds, (const u8*)&wfds, sizeof(fd_set));
        if(exceptfds.address() != 0) mmu_.copyToMmu(exceptfds, (const u8*)&efds, sizeof(fd_set));
        if(timeout.address() != 0) mmu_.copyToMmu(timeout, (const u8*)&ts, sizeof(timespec));
        return ret;
    }

    int Sys::ppoll(x64::Ptr fds, int nfds, x64::Ptr tmo_p, x64::Ptr sigmask, size_t sigsetsize) {
        verify(!sigmask, "Sys::ppoll does not support non-null sigmask");
        assert(sizeof(FS::PollData) == Host::pollRequiredBufferSize(1));
        std::vector<FS::PollData> pollfds = mmu_.readFromMmu<FS::PollData>(fds, nfds);
        Timer* timer = kernel_.timers().getOrTryCreate(0);
        verify(!!timer);
        auto timeoutDuration = timer->readRelativeTimespec(mmu_, tmo_p);
        int timeoutInMs = -1;
        if(!!timeoutDuration) {
            timeoutInMs = (int)(timeoutDuration->seconds*1'000 + timeoutDuration->nanoseconds / 1'000'000);
        }
        if(logSyscalls_) {
            print("Sys::ppoll(fds={:#x}, nfds={}, timeout={:#x}, sigmask={:#x}, sigsetsize={}) = pending\n",
                        fds.address(), nfds, tmo_p.address(), sigmask.address(), sigsetsize);
        }
        kernel_.scheduler().poll(currentThread_, fds, nfds, timeoutInMs);
        return 0;
    }

    long Sys::set_robust_list(x64::Ptr head, size_t len) {
        if(logSyscalls_) print("Sys::set_robust_list({:#x}, {}) = 0\n", head.address(), len);
        currentThread_->setRobustList(head, len);
        return 0;
    }

    long Sys::get_robust_list(int pid, x64::Ptr64 head_ptr, x64::Ptr64 len_ptr) {
        if(logSyscalls_) print("Sys::get_robust_list({}, {:#x}, {:#x}) = 0\n", pid, head_ptr.address(), len_ptr.address());
        (void)pid;
        (void)head_ptr;
        (void)len_ptr;
        verify(false, "implement {get,set}_robust_list");
        return 0;
    }

    int Sys::utimensat(int dirfd, x64::Ptr pathname, x64::Ptr times, int flags) {
        if(logSyscalls_) print("Sys::utimensat(dirfd={}, pathname={}, times={:#x}, flags={}) = -ENOTSUP\n",
                                                          dirfd, mmu_.readString(pathname), times.address(), flags);
        warn(fmt::format("utimensat not implemented"));
        return -ENOTSUP;
    }

    int Sys::fallocate(int fd, int mode, off_t offset, off_t len) {
        int ret = kernel_.fs().fallocate(FS::FD{fd}, mode, offset, len);
        if(logSyscalls_) {
            print("Sys::fallocate(fd={}, mode={}, offset={:#x}, len={}) = {}\n",
                                                          fd, mode, offset, len, ret);
        }
        return ret;
    }

    int Sys::eventfd2(unsigned int initval, int flags) {
        FS::FD fd = kernel_.fs().eventfd2(initval, flags);
        if(logSyscalls_) {
            print("Sys::eventfd2(initval={}, flags={}) = {}\n", initval, flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::epoll_create1(int flags) {
        FS::FD fd = kernel_.fs().epoll_create1(flags);
        if(logSyscalls_) {
            print("Sys::epoll_create1(flags={}) = {}\n", flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::dup3(int oldfd, int newfd, int flags) {
        FS::FD fd = kernel_.fs().dup3(FS::FD{oldfd}, FS::FD{newfd}, flags);
        if(logSyscalls_) {
            print("Sys::dup3(oldfd={}, newfd={}, flags={}) = {}\n", oldfd, newfd, flags, fd.fd);
        }
        return fd.fd;
    }
    
    int Sys::pipe2(x64::Ptr32 pipefd, int flags) {
        auto errnoOrFds = kernel_.fs().pipe2(flags);
        int ret = errnoOrFds.errorOrWith<int>([&](std::pair<FS::FD, FS::FD> fds) {
            std::vector<u32> fdsbuf {{ (u32)fds.first.fd, (u32)fds.second.fd }};
            x64::Ptr ptr { pipefd.address() };
            mmu_.writeToMmu(ptr, fdsbuf);
            return 0;
        });
        if(logSyscalls_) {
            print("Sys::pipe2(pipefd={:#x}, flags={}) = {}\n", pipefd.address(), flags, ret);
        }
        return ret;
    }

    int Sys::inotify_init1(int flags) {
        if(logSyscalls_) {
            print("Sys::inotify_init1(flags={}) = {}\n", flags, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    int Sys::prlimit64(pid_t pid, int resource, x64::Ptr new_limit, x64::Ptr old_limit) {
        if(logSyscalls_) 
            print("Sys::prlimit64(pid={}, resource={}, new_limit={:#x}, old_limit={:#x})", pid, resource, new_limit.address(), old_limit.address());
        if(!old_limit.address()) {
            if(logSyscalls_) print(" = 0\n");
            return 0;
        }
        auto errnoOrBuffer = Host::getrlimit(pid, resource);
        if(logSyscalls_) print(" = {}\n", errnoOrBuffer.errorOr(0));
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(old_limit, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::sched_setattr(pid_t pid, x64::Ptr attr, unsigned int flags) {
        if(logSyscalls_) {
            Host::SchedAttr attributes = mmu_.readFromMmu<Host::SchedAttr>(attr);
            std::string attributeString = fmt::format("policy={} flags={} nice={} priority={}", attributes.schedPolicy, attributes.schedFlags, attributes.schedNice, attributes.schedPriority);
            print("Sys::sched_setattr(pid={}, attr={:#x} ({}), flags={:#x}) = 0", pid, attr.address(), attributeString, flags);
        }
        return 0;
    }

    int Sys::sched_getattr(pid_t pid, x64::Ptr attr, unsigned int size, unsigned int flags) {
        Host::SchedAttr attributes = Host::getSchedulerAttributes();
        if(size < sizeof(attributes)) {
            if(logSyscalls_) 
                print("Sys::sched_getattr(pid={}, attr={:#x}, size={:#x}, flags={:#x}) = {}", pid, attr.address(), size, flags, -EINVAL);
            return -EINVAL;
        }
        mmu_.writeToMmu<Host::SchedAttr>(attr, attributes);
        if(logSyscalls_) 
            print("Sys::sched_getattr(pid={}, attr={:#x}, size={:#x}, flags={:#x}) = 0", pid, attr.address(), size, flags);
        return 0;
    }

    ssize_t Sys::getrandom(x64::Ptr buf, size_t len, int flags) {
        if(logSyscalls_) 
            print("Sys::getrandom(buf={:#x}, len={}, flags={})\n", buf.address(), len, flags);
        std::vector<u8> buffer(len);
        std::iota(buffer.begin(), buffer.end(), 0);
        mmu_.copyToMmu(buf, buffer.data(), buffer.size());
        return (ssize_t)len;
    }

    int Sys::memfd_create(x64::Ptr name, unsigned int flags) {
        auto filename = mmu_.readString(name);
        FS::FD fd = kernel_.fs().memfd_create(filename, flags);
        if(logSyscalls_) {
            std::string pathname = mmu_.readString(name);
            print("Sys::memfd_create(name={}, flags={:#x}) = {}\n", pathname, flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::statx(int dirfd, x64::Ptr pathname, int flags, unsigned int mask, x64::Ptr statxbuf) {
        std::string path = mmu_.readString(pathname);
        auto errnoOrBuffer = kernel_.fs().statx(FS::FD{dirfd}, path, flags, mask);
        if(logSyscalls_) {
            print("Sys::statx(dirfd={}, path={}, flags={}, mask={}, statxbuf={:#x}) = {}\n",
                        dirfd, path, flags, mask, statxbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_.copyToMmu(statxbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::clone3(x64::Ptr uargs, size_t size) {
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
        std::vector<u64> args = mmu_.readFromMmu<u64>(uargs, size / sizeof(u64));
        verify(args.size() >= 8);
        x64::Ptr32 child_tid { args[2] };
        u64 stackAddress = args[5] + args[6];
        u64 tls = args[7];

        Thread* currentThread = currentThread_;
        std::unique_ptr<Thread> newThread = kernel_.scheduler().allocateThread(currentThread->description().pid);
        const Thread::SavedCpuState& oldCpuState = currentThread->savedCpuState();
        Thread::SavedCpuState& newCpuState = newThread->savedCpuState();
        newCpuState.regs = oldCpuState.regs;
        newCpuState.regs.set(x64::R64::RAX, 0);
        newCpuState.regs.rip() = oldCpuState.regs.rip();
        newCpuState.regs.rsp() = stackAddress;
        mmu_.setRegionName(stackAddress, fmt::format("Stack of thread {}", newThread->description().tid));
        newCpuState.fsBase = tls;
        newThread->setClearChildTid(child_tid);
        long ret = newThread->description().tid;
        if(!!child_tid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_.write32(child_tid, (u32)ret);
        }
        if(logSyscalls_) {
            print("Sys::clone3(uargs={:#x}, size={}) = {}\n",
                        uargs.address(), size, ret);
        }
        kernel_.scheduler().addThread(std::move(newThread));
        return (int)ret;
    }

}