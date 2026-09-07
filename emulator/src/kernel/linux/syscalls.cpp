#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"
#include "kernel/linux/shm/sharedmemory.h"
#include "kernel/linux/sys/execve.h"
#include "kernel/linux/kernel.h"
#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/scheduler.h"
#include "kernel/linux/syscallenums.h"
#include "kernel/linux/syscalls.h"
#include "kernel/linux/thread.h"
#include "host/host.h"
#include "mem/mmu.h"
#include "scopeguard.h"
#include "verify.h"
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <numeric>
#include <sys/socket.h>

namespace kernel::gnulinux {

    Sys::Sys(Kernel& kernel) :
        kernel_(kernel) {
    }

    template<typename... Args>
    void Sys::print(const char* format, Args... args) const {
        fmt::print("[{}:{}@{:#12x}] ", currentThread_->description().pid, currentThread_->description().tid, currentThread_->time().nbInstructions());
        fmt::print(format, args...);
        fmt::println("");
        [[maybe_unused]]int ret = fflush(stdout);
    }

    template<typename... Args>
    void Sys::warn(const char* format, Args... args) const {
        fmt::print(fg(fmt::color::red), "[{}:{}@{:#12x}] ", currentThread_->description().pid, currentThread_->description().tid, currentThread_->time().nbInstructions());
        fmt::print(fg(fmt::color::red), format, args...);
        fmt::println("");
        [[maybe_unused]]int ret = fflush(stdout);
    }

    void Sys::syscall(Process* process, Thread* thread) {
        std::scoped_lock<std::mutex> lock(mutex_);
        mem::Mmu mmu(process->addressSpace());
        currentProcess_ = process;
        currentThread_ = thread;
        mmu_ = &mmu;
        mmu.addCallback(process);
        ScopeGuard scopeGuard([&]() {
            currentProcess_ = nullptr;
            currentThread_ = nullptr;
            mmu_ = nullptr;
            mmu.removeCallback(process);
        });

        SYSCALL sysNumber {};
        RegisterDump regs;
        currentThread_->loadSyscallInput(&sysNumber, Span<u64>(regs.args.data(), regs.args.data() + regs.args.size()));

        switch(sysNumber) {
            case SYSCALL::READ: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::read, regs));
            case SYSCALL::WRITE: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::write, regs));
            case SYSCALL::OPEN: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::open, regs));
            case SYSCALL::CLOSE: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::close, regs));
            case SYSCALL::STAT: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::stat, regs));
            case SYSCALL::FSTAT: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::fstat, regs));
            case SYSCALL::LSTAT: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::lstat, regs));
            case SYSCALL::POLL: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::poll, regs));
            case SYSCALL::LSEEK: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::lseek, regs));
            case SYSCALL::MMAP: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::mmap, regs));
            case SYSCALL::MPROTECT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::mprotect, regs));
            case SYSCALL::MUNMAP: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::munmap, regs));
            case SYSCALL::BRK: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::brk, regs));
            case SYSCALL::RT_SIGACTION: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::rt_sigaction, regs));
            case SYSCALL::RT_SIGPROCMASK: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::rt_sigprocmask, regs));
            case SYSCALL::IOCTL: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::ioctl, regs));
            case SYSCALL::PREAD64: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::pread64, regs));
            case SYSCALL::PWRITE64: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::pwrite64, regs));
            case SYSCALL::READV: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::readv, regs));
            case SYSCALL::WRITEV: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::writev, regs));
            case SYSCALL::ACCESS: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::access, regs));
            case SYSCALL::PIPE: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::pipe, regs));
            case SYSCALL::SELECT: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::select, regs));
            case SYSCALL::SCHED_YIELD: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::sched_yield, regs));
            case SYSCALL::MREMAP: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::mremap, regs));
            case SYSCALL::MSYNC: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::msync, regs));
            case SYSCALL::MINCORE: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::mincore, regs));
            case SYSCALL::MADVISE: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::madvise, regs));
            case SYSCALL::SHMGET: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::shmget, regs));
            case SYSCALL::SHMAT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::shmat, regs));
            case SYSCALL::SHMCTL: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::shmctl, regs));
            case SYSCALL::DUP: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::dup, regs));
            case SYSCALL::DUP2: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::dup2, regs));
            case SYSCALL::SETITIMER: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::setitimer, regs));
            case SYSCALL::GETPID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getpid, regs));
            case SYSCALL::SOCKET: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::socket, regs));
            case SYSCALL::CONNECT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::connect, regs));
            case SYSCALL::SENDTO: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::sendto, regs));
            case SYSCALL::RECVFROM: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::recvfrom, regs));
            case SYSCALL::SENDMSG: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::sendmsg, regs));
            case SYSCALL::RECVMSG: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::recvmsg, regs));
            case SYSCALL::SHUTDOWN: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::shutdown, regs));
            case SYSCALL::BIND: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::bind, regs));
            case SYSCALL::LISTEN: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::listen, regs));
            case SYSCALL::GETSOCKNAME: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getsockname, regs));
            case SYSCALL::GETPEERNAME: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getpeername, regs));
            case SYSCALL::SOCKETPAIR: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::socketpair, regs));
            case SYSCALL::SETSOCKOPT: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::setsockopt, regs));
            case SYSCALL::GETSOCKOPT: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::getsockopt, regs));
            case SYSCALL::CLONE: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::clone, regs));
            case SYSCALL::FORK: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::fork, regs));
            case SYSCALL::VFORK: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::vfork, regs));
            case SYSCALL::EXECVE: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::execve, regs));
            case SYSCALL::EXIT: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::exit, regs));
            case SYSCALL::WAIT4: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::wait4, regs));
            case SYSCALL::KILL: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::kill, regs));
            case SYSCALL::UNAME: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::uname, regs));
            case SYSCALL::SHMDT: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::shmdt, regs));
            case SYSCALL::FCNTL: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::fcntl, regs));
            case SYSCALL::FLOCK: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::flock, regs));
            case SYSCALL::FSYNC: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::fsync, regs));
            case SYSCALL::FDATASYNC: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::fdatasync, regs));
            case SYSCALL::TRUNCATE: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::truncate, regs));
            case SYSCALL::FTRUNCATE: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::ftruncate, regs));
            case SYSCALL::GETCWD: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::getcwd, regs));
            case SYSCALL::CHDIR: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::chdir, regs));
            case SYSCALL::RENAME: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::rename, regs));
            case SYSCALL::MKDIR: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::mkdir, regs));
            case SYSCALL::UNLINK: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::unlink, regs));
            case SYSCALL::READLINK: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::readlink, regs));
            case SYSCALL::CHMOD: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::chmod, regs));
            case SYSCALL::FCHMOD: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::fchmod, regs));
            case SYSCALL::CHOWN: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::chown, regs));
            case SYSCALL::FCHOWN: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::fchown, regs));
            case SYSCALL::UMASK: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::umask, regs));
            case SYSCALL::GETTIMEOFDAY: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::gettimeofday, regs));
            case SYSCALL::GETRUSAGE: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::getrusage, regs));
            case SYSCALL::SYSINFO: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::sysinfo, regs));
            case SYSCALL::TIMES: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::times, regs));
            case SYSCALL::GETUID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getuid, regs));
            case SYSCALL::GETGID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getgid, regs));
            case SYSCALL::GETEUID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::geteuid, regs));
            case SYSCALL::GETEGID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getegid, regs));
            case SYSCALL::SETPGID: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::setpgid, regs));
            case SYSCALL::GETPPID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getppid, regs));
            case SYSCALL::GETPGRP: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::getpgrp, regs));
            case SYSCALL::GETGROUPS: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::getgroups, regs));
            case SYSCALL::GETRESUID: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getresuid, regs));
            case SYSCALL::GETRESGID: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getresgid, regs));
            case SYSCALL::GETPGID: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::getpgid, regs));
            case SYSCALL::RT_SIGTIMEDWAIT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::rt_sigtimedwait, regs));
            case SYSCALL::SIGALTSTACK: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::sigaltstack, regs));
            case SYSCALL::UTIME: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::utime, regs));
            case SYSCALL::STATFS: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::statfs, regs));
            case SYSCALL::FSTATFS: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::fstatfs, regs));
            case SYSCALL::GETPRIORITY: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::getpriority, regs));
            case SYSCALL::SETPRIORITY: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::setpriority, regs));
            case SYSCALL::SCHED_GETPARAM: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::sched_getparam, regs));
            case SYSCALL::SCHED_SETSCHEDULER: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::sched_setscheduler, regs));
            case SYSCALL::SCHED_GETSCHEDULER: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::sched_getscheduler, regs));
            case SYSCALL::SCHED_GET_PRIORITY_MAX: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::sched_get_priority_max, regs));
            case SYSCALL::SCHED_GET_PRIORITY_MIN: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::sched_get_priority_min, regs));
            case SYSCALL::MLOCK: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::mlock, regs));
            case SYSCALL::MUNLOCK: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::munlock, regs));
            case SYSCALL::MLOCKALL: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::mlockall, regs));
            case SYSCALL::MUNLOCKALL: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::munlockall, regs));
            case SYSCALL::PRCTL: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::prctl, regs));
            case SYSCALL::ARCH_PRCTL: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::arch_prctl, regs));
            case SYSCALL::GETTID: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::gettid, regs));
            case SYSCALL::GETXATTR: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::getxattr, regs));
            case SYSCALL::LGETXATTR: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::lgetxattr, regs));
            case SYSCALL::LISTXATTR: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::listxattr, regs));
            case SYSCALL::TIME: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::time, regs));
            case SYSCALL::FUTEX: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::futex, regs));
            case SYSCALL::SCHED_SETAFFINITY: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::sched_setaffinity, regs));
            case SYSCALL::SCHED_GETAFFINITY: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::sched_getaffinity, regs));
            case SYSCALL::GETDENTS64: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getdents64, regs));
            case SYSCALL::SET_TID_ADDRESS: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::set_tid_address, regs));
            case SYSCALL::POSIX_FADVISE: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::posix_fadvise, regs));
            case SYSCALL::CLOCK_GETTIME: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::clock_gettime, regs));
            case SYSCALL::CLOCK_GETRES: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::clock_getres, regs));
            case SYSCALL::CLOCK_NANOSLEEP: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::clock_nanosleep, regs));
            case SYSCALL::EXIT_GROUP: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::exit_group, regs));
            case SYSCALL::EPOLL_WAIT: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::epoll_wait, regs));
            case SYSCALL::EPOLL_CTL: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::epoll_ctl, regs));
            case SYSCALL::TGKILL: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::tgkill, regs));
            case SYSCALL::MBIND: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::mbind, regs));
            case SYSCALL::GET_MEMPOLICY: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::get_mempolicy, regs));
            case SYSCALL::WAITID: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::waitid, regs));
            case SYSCALL::INOTIFY_INIT: return currentThread_->setSyscallOutput(invoke_syscall_0(&Sys::inotify_init, regs));
            case SYSCALL::INOTIFY_ADD_WATCH: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::inotify_add_watch, regs));
            case SYSCALL::OPENAT: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::openat, regs));
            case SYSCALL::FSTATAT64: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::fstatat64, regs));
            case SYSCALL::UNLINKAT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::unlinkat, regs));
            case SYSCALL::LINKAT: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::linkat, regs));
            case SYSCALL::READLINKAT: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::readlinkat, regs));
            case SYSCALL::FACCESSAT: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::faccessat, regs));
            case SYSCALL::PSELECT6: return currentThread_->setSyscallOutput(invoke_syscall_6(&Sys::pselect6, regs));
            case SYSCALL::PPOLL: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::ppoll, regs));
            case SYSCALL::SET_ROBUST_LIST: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::set_robust_list, regs));
            case SYSCALL::GET_ROBUST_LIST: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::get_robust_list, regs));
            case SYSCALL::UTIMENSAT: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::utimensat, regs));
            case SYSCALL::FALLOCATE: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::fallocate, regs));
            case SYSCALL::EVENTFD2: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::eventfd2, regs));
            case SYSCALL::EPOLL_CREATE1: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::epoll_create1, regs));
            case SYSCALL::DUP3: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::dup3, regs));
            case SYSCALL::PIPE2: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::pipe2, regs));
            case SYSCALL::INOTIFY_INIT1: return currentThread_->setSyscallOutput(invoke_syscall_1(&Sys::inotify_init1, regs));
            case SYSCALL::PRLIMIT64: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::prlimit64, regs));
            case SYSCALL::SCHED_SETATTR: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::sched_setattr, regs));
            case SYSCALL::SCHED_GETATTR: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::sched_getattr, regs));
            case SYSCALL::GETRANDOM: return currentThread_->setSyscallOutput(invoke_syscall_3(&Sys::getrandom, regs));
            case SYSCALL::MEMFD_CREATE: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::memfd_create, regs));
            case SYSCALL::STATX: return currentThread_->setSyscallOutput(invoke_syscall_5(&Sys::statx, regs));
            case SYSCALL::RSEQ: return currentThread_->setSyscallOutput(invoke_syscall_4(&Sys::rseq, regs));
            case SYSCALL::CLONE3: return currentThread_->setSyscallOutput(invoke_syscall_2(&Sys::clone3, regs));
        }
    }

    ssize_t Sys::read(int fd, mem::Ptr8 buf, size_t count) {
        auto descriptor = currentProcess_->fds()[fd];
        auto readResult = kernel_.fs().read(descriptor, count);

        if(readResult.isBlocking()) {
            kernel_.scheduler().blockingRead(currentThread_, fd, buf, count);
            if(kernel_.logSyscalls()) {
                print("Sys::read(fd={}, buf={:#x}, count={}) = blocked",
                        fd, buf.address(), count);
            }
            return 0;
        }

        verify(!readResult.isBlocking(), "blocking read not handled in Sys::read");
        
        ssize_t ret = readResult.value().errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });

        if(kernel_.logSyscalls()) {
            print("Sys::read(fd={}, buf={:#x}, count={}) = {}",
                    fd, buf.address(), count, ret);
        }
        return ret;
    }

    ssize_t Sys::write(int fd, mem::Ptr8 buf, size_t count) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        auto descriptor = currentProcess_->fds()[fd];
        ssize_t ret = kernel_.fs().write(descriptor, buffer.data(), buffer.size());
        if(kernel_.logSyscalls()) {
            print("Sys::write(fd={}, buf={:#x}, count={}) = {}",
                        fd, buf.address(), count, ret);
        }
        return ret;
    }

    int Sys::open(mem::Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_->readString(pathname);
        BitFlags<AccessMode> accessMode = FS::toAccessMode(flags);
        BitFlags<CreationFlags> creationFlags = FS::toCreationFlags(flags);
        BitFlags<StatusFlags> statusFlags = FS::toStatusFlags(flags);
        Permissions permissions = FS::fromMode(mode);
        auto dirFd = currentProcess_->cwd();
        auto filepath = kernel_.fs().resolvePath(dirFd, path);
        FD fd = [&]() -> FD {
            if(!filepath) return FD{-ENOENT};
            return currentProcess_->fds().open(*filepath, accessMode, creationFlags, statusFlags, permissions);
        }();
        if(kernel_.logSyscalls()) {
            std::string flagsString = fmt::format("[{}{}{}{}{}{}{}]",
                accessMode.test(AccessMode::READ)  ? "Read " : "",
                accessMode.test(AccessMode::WRITE) ? "Write " : "",
                statusFlags.test(StatusFlags::APPEND) ? "Append " : "",
                creationFlags.test(CreationFlags::TRUNC) ? "Truncate " : "",
                creationFlags.test(CreationFlags::CREAT) ? "Create " : "",
                creationFlags.test(CreationFlags::CLOEXEC) ? "CloseOnExec " : "",
                creationFlags.test(CreationFlags::DIRECTORY) ? "Directory " : "");
            print("Sys::open(path={}, flags={}, mode={:o}) = {}", path, flagsString, mode, fd.fd);
        }
        return fd.fd;
    }

    int Sys::close(int fd) {
        int ret = currentProcess_->fds().close(FD{fd});
        if(kernel_.logSyscalls()) print("Sys::close(fd={}) = {}", fd, ret);
        return ret;
    }

    int Sys::stat(mem::Ptr pathname, mem::Ptr statbuf) {
        std::string pathname_ = mmu_->readString(pathname);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), pathname_);
        auto errnoOrBuffer = [&]() {
            if(!path) return ErrnoOrBuffer(-ENOENT);
            return kernel_.fs().stat(*path);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::stat(path={}, statbuf={:#x}) = {}",
                        pathname_, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstat(int fd, mem::Ptr8 statbuf) {
        auto descriptor = currentProcess_->fds()[fd];
        ErrnoOrBuffer errnoOrBuffer = kernel_.fs().fstat(descriptor);
        if(kernel_.logSyscalls()) {
            print("Sys::fstat(fd={}, statbuf={:#x}) = {}",
                        fd, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::lstat(mem::Ptr pathname, mem::Ptr statbuf) {
        std::string path = mmu_->readString(pathname);
        ErrnoOrBuffer errnoOrBuffer = Host::lstat(path);
        if(kernel_.logSyscalls()) {
            print("Sys::lstat(path={}, statbuf={:#x}) = {}",
                        path, statbuf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::poll(mem::Ptr fds, size_t nfds, int timeout) {
        assert(sizeof(FS::PollFd) == Host::pollRequiredBufferSize(1));
        std::vector<FS::PollFd> pollfds = mmu_->readFromMmu<FS::PollFd>(fds, nfds);
        if(timeout == 0) {
            std::vector<FS::PollData> polldata;
            polldata.reserve(pollfds.size());
            for(auto pollfd : pollfds) {
                polldata.push_back(FS::PollData {
                    pollfd.fd,
                    currentProcess_->fds()[pollfd.fd],
                    pollfd.events,
                    pollfd.revents,
                });
            }
            auto errnoOrBufferAndReturnValue = kernel_.fs().pollImmediate(polldata);
            if(kernel_.logSyscalls()) {
                std::vector<std::string> allfds;
                for(const auto& pfd : pollfds) allfds.push_back(fmt::format("[fd={}, events={}]", pfd.fd, (int)pfd.events));
                auto fdsString = fmt::format("{}", fmt::join(allfds, ", "));
                print("Sys::poll(fds={:#x}, nfds={} (fds={}), timeout={}) = {}",
                            fds.address(), nfds, fdsString, timeout, errnoOrBufferAndReturnValue.errorOr(0));
                for(const auto& pd : pollfds) {
                    fmt::print("  fd={}  events={}, revents={}", pd.fd, (int)pd.events, (int)pd.revents);
                }
            }
            return errnoOrBufferAndReturnValue.errorOrWith<int>([&](const auto& bufferAndRetVal) {
                mmu_->copyToMmu(fds, bufferAndRetVal.buffer.data(), bufferAndRetVal.buffer.size());
                return bufferAndRetVal.returnValue;
            });
        } else {
            if(kernel_.logSyscalls()) {
                std::vector<std::string> allfds;
                for(const auto& pfd : pollfds) allfds.push_back(fmt::format("[fd={}, events={}]", pfd.fd, (int)pfd.events));
                auto fdsString = fmt::format("{}", fmt::join(allfds, ", "));
                print("Sys::poll(fds={:#x}, nfds={} (fds={}), timeout={}) = pending",
                            fds.address(), nfds, fdsString, timeout);
            }
            kernel_.scheduler().poll(currentThread_, fds, nfds, timeout);
        }
        return 0;
    }

    off_t Sys::lseek(int fd, off_t offset, int whence) {
        auto descriptor = currentProcess_->fds()[fd];
        off_t ret = kernel_.fs().lseek(descriptor, offset, whence);
        if(kernel_.logSyscalls()) print("Sys::lseek(fd={}, offset={:#x}, whence={}) = {}", fd, offset, whence, ret);
        return ret;
    }

    mem::Ptr Sys::mmap(mem::Ptr addr, size_t length, int prot, int flags, int fd, off_t offset) {
        BitFlags<mem::MAP> mmapFlags;
        if(Host::Mmap::isAnonymous(flags)) mmapFlags.add(mem::MAP::ANONYMOUS);
        if(Host::Mmap::isFixed(flags)) mmapFlags.add(mem::MAP::FIXED);
        if(Host::Mmap::isFixedNoReplace(flags)) {
            mmapFlags.add(mem::MAP::FIXED);
            mmapFlags.add(mem::MAP::NO_REPLACE);
        }
        if(Host::Mmap::isPrivate(flags)) mmapFlags.add(mem::MAP::PRIVATE);
        if(Host::Mmap::isShared(flags)) mmapFlags.add(mem::MAP::SHARED);

        BitFlags<mem::PROT> protFlags = BitFlags<mem::PROT>::fromIntegerType(prot);

        if(mmapFlags.test(mem::MAP::SHARED) && protFlags.test(mem::PROT::WRITE)) {
            warn("mmap: writable and shared mapping not supported. Making mapping private.");
            mmapFlags.remove(mem::MAP::SHARED);
            mmapFlags.add(mem::MAP::PRIVATE);
        }

        auto base = mmu_->mmap(addr.address(), length, protFlags, mmapFlags);
        if(base && !mmapFlags.test(mem::MAP::ANONYMOUS)) {
            u64 regionBase = base.value();
            verify(fd >= 0);
            auto descriptor = currentProcess_->fds()[fd];
            ErrnoOrBuffer data = kernel_.fs().pread(descriptor, length, offset);
            if(data.isError()) {
                auto filename = kernel_.fs().filename(descriptor);
                warn("mmap: could not mmap file \"{}\" with fd={}", filename, fd);
                base = (u64)data.errorOr(0);
            }
            data.errorOrWith<int>([&](const Buffer& buffer) {
                BitFlags<mem::PROT> saved = mmu_->prot(regionBase);
                BitFlags<mem::PROT> savedAndWriteable = saved;
                savedAndWriteable.add(mem::PROT::WRITE);
                savedAndWriteable.remove(mem::PROT::EXEC);
                verify(mmu_->mprotect(regionBase, length, savedAndWriteable) >= 0, "mprotect failed");
                mmu_->copyToMmu(mem::Ptr8{regionBase}, buffer.data(), buffer.size());
                verify(mmu_->mprotect(regionBase, length, saved) >= 0, "mprotect failed");
                auto filename = kernel_.fs().filename(descriptor);
                mmu_->setRegionName(regionBase, filename);
                return 0;
            });
        }
        if(kernel_.logSyscalls()) {
            BitFlags<mem::PROT> protFlags = BitFlags<mem::PROT>::fromIntegerType(prot);
            bool protRead = protFlags.test(mem::PROT::READ);
            bool protWrite = protFlags.test(mem::PROT::WRITE);
            bool protExec = protFlags.test(mem::PROT::EXEC);
            std::string protString = fmt::format("{}{}{}",
                    protRead  ? "R" : "",
                    protWrite ? "W" : "",
                    protExec  ? "X" : "");
            std::string flagsString = fmt::format("{}{}{}{}",
                    mmapFlags.test(mem::MAP::ANONYMOUS) ? "ANONYMOUS " : "",
                    mmapFlags.test(mem::MAP::FIXED) ? "FIXED " : "",
                    mmapFlags.test(mem::MAP::PRIVATE) ? "PRIVATE " : "",
                    mmapFlags.test(mem::MAP::SHARED) ? "SHARED " : "");
            print("Sys::mmap(addr={:#x}, length={}, prot={}, flags={}, fd={}, offset={}) = {:#x}",
                    addr.address(), length, protString, flagsString, fd, offset, base.value_or(-ENOMEM));
        }
        return mem::Ptr{base.value_or(-ENOMEM)};
    }

    int Sys::mprotect(mem::Ptr addr, size_t length, int prot) {
        BitFlags<mem::PROT> protFlags = BitFlags<mem::PROT>::fromIntegerType(prot);
        int ret = mmu_->mprotect(addr.address(), length, protFlags);
        if(kernel_.logSyscalls()) {
            bool protRead = protFlags.test(mem::PROT::READ);
            bool protWrite = protFlags.test(mem::PROT::WRITE);
            bool protExec = protFlags.test(mem::PROT::EXEC);
            std::string protString = fmt::format("{}{}{}",
                    protRead  ? "R" : "",
                    protWrite ? "W" : "",
                    protExec  ? "X" : "");
            print("Sys::mprotect(addr={:#x}, length={}, prot={}) = {}", addr.address(), length, protString, ret);
        }
        return ret;
    }

    int Sys::munmap(mem::Ptr addr, size_t length) {
        int ret = mmu_->munmap(addr.address(), length);
        if(kernel_.logSyscalls()) print("Sys::munmap(addr={:#x}, length={}) = {}", addr.address(), length, ret);
        return ret;
    }

    mem::Ptr Sys::brk(mem::Ptr addr) {
        u64 newBrk = mmu_->brk(addr.address());
        if(kernel_.logSyscalls()) print("Sys::brk(addr={:#x}) = {:#x}", addr.address(), newBrk);
        return mem::Ptr{newBrk};
    }

    int Sys::rt_sigaction(int sig, mem::Ptr act, mem::Ptr oact, size_t sigsetsize) {
        if(kernel_.logSyscalls()) print("Sys::rt_sigaction({}, {:#x}, {:#x}, {}) = 0", sig, act.address(), oact.address(), sigsetsize);
        (void)sig;
        (void)act;
        (void)oact;
        (void)sigsetsize;
        return 0;
    }

    int Sys::rt_sigprocmask(int how, mem::Ptr nset, mem::Ptr oset, size_t sigsetsize) {
        if(kernel_.logSyscalls()) print("Sys::rt_sigprocmask({}, {:#x}, {:#x}, {}) = 0", how, nset.address(), oset.address(), sigsetsize);
        (void)how;
        (void)nset;
        (void)oset;
        (void)sigsetsize;
        return 0;
    }

    int Sys::ioctl(int fd, unsigned long request, mem::Ptr argp) {
        // We need to ask the host for the expected buffer size behind argp.
        auto bufferSize = Host::ioctlRequiredBufferSize(request);
        if(!bufferSize) {
            if(kernel_.logSyscalls()) {
                print("Sys::ioctl(fd={}, request={}, argp={:#x}) = {}",
                            fd, Host::ioctlName(request), argp.address(), -EINVAL);
            }
            warn("Unknown ioctl {:#x}. Returning -EINVAL", request);
            return -EINVAL;
        };
        Buffer buffer(bufferSize.value(), 0x0);
        mmu_->copyFromMmu(buffer.data(), argp, buffer.size());

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
            if(Host::Ioctl::isTIOCSPGRP(hostRequest)) return Ioctl::tiocspgrp;
            return {};
        }(request);
        verify(!!fsrequest, "Unknown request");
        auto descriptor = currentProcess_->fds()[fd];
        auto errnoOrBuffer = kernel_.fs().ioctl(descriptor, fsrequest.value(), buffer);
        if(kernel_.logSyscalls()) {
            print("Sys::ioctl(fd={}, request={}, argp={:#x}) = {}",
                        fd, Host::ioctlName(request), argp.address(),
                        errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            // The buffer returned by ioctl is empty when nothing needs to be written back.
            mmu_->copyToMmu(argp, buffer.data(), buffer.size());
            return 0;
        });
    }

    ssize_t Sys::pread64(int fd, mem::Ptr buf, size_t count, off_t offset) {
        auto descriptor = currentProcess_->fds()[fd];
        auto errnoOrBuffer = kernel_.fs().pread(descriptor, count, offset);
        if(kernel_.logSyscalls()) {
            print("Sys::pread64(fd={}, buf={:#x}, count={}, offset={}) = {}",
                        fd, buf.address(), count, offset,
                        errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buf) { return (ssize_t)buf.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::pwrite64(int fd, mem::Ptr buf, size_t count, off_t offset) {
        std::vector<u8> buffer = mmu_->readFromMmu<u8>(buf, count);
        auto descriptor = currentProcess_->fds()[fd];
        auto errnoOrNbytes = kernel_.fs().pwrite(descriptor, buffer.data(), buffer.size(), offset);
        if(kernel_.logSyscalls()) {
            print("Sys::pwrite64(fd={}, buf={:#x}, count={}, offset={}) = {}",
                        fd, buf.address(), count, offset, errnoOrNbytes);
        }
        return errnoOrNbytes;
    }

    ssize_t Sys::readv(int fd, mem::Ptr iov, int iovcnt) {
        Buffer iovecBuffer(((size_t)iovcnt) * Host::iovecRequiredBufferSize(), 0x0);
        mmu_->copyFromMmu(iovecBuffer.data(), iov, iovecBuffer.size());
        std::vector<Buffer> buffers;
        buffers.reserve((size_t)iovcnt);
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            mem::Ptr base{Host::iovecBase(iovecBuffer, i)};
            size_t len = Host::iovecLen(iovecBuffer, i);
            Buffer data(len, 0x0);
            mmu_->copyFromMmu(data.data(), base, len);
            buffers.push_back(Buffer(std::move(data)));
        }
        auto descriptor = currentProcess_->fds()[fd];
        ssize_t nbytes = kernel_.fs().readv(descriptor, &buffers);
        if(nbytes >= 0) {
            for(size_t i = 0; i < (size_t)iovcnt; ++i) {
                mem::Ptr base{Host::iovecBase(iovecBuffer, i)};
                mmu_->copyToMmu(base, buffers[i].data(), buffers[i].size());
            }
        }
        if(kernel_.logSyscalls()) print("Sys::readv(fd={}, iov={:#x}, iovcnt={}) = {}", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    ssize_t Sys::writev(int fd, mem::Ptr iov, int iovcnt) {
        Buffer iovecs(((size_t)iovcnt) * Host::iovecRequiredBufferSize(), 0x0);
        mmu_->copyFromMmu(iovecs.data(), iov, iovecs.size());
        Buffer iovecBuffer(std::move(iovecs));
        std::vector<Buffer> buffers;
        for(size_t i = 0; i < (size_t)iovcnt; ++i) {
            mem::Ptr base{Host::iovecBase(iovecBuffer, i)};
            size_t len = Host::iovecLen(iovecBuffer, i);
            Buffer data(len, 0x0);
            mmu_->copyFromMmu(data.data(), base, len);
            buffers.push_back(Buffer(std::move(data)));
        }
        auto descriptor = currentProcess_->fds()[fd];
        ssize_t nbytes = kernel_.fs().writev(descriptor, buffers);
        if(kernel_.logSyscalls()) print("Sys::writev(fd={}, iov={:#x}, iovcnt={}) = {}", fd, iov.address(), iovcnt, nbytes);
        return nbytes;
    }

    int Sys::access(mem::Ptr pathname, int mode) {
        std::string pathname_ = mmu_->readString(pathname);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), pathname_);
        int ret = [&]() {
            if(!path) return -ENOENT;
            return kernel_.fs().access(*path, mode);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::access(path={}, mode={}) = {}", pathname_, mode, ret);
        }
        return ret;
    }
    
    int Sys::pipe(mem::Ptr32 pipefd) {
        auto errnoOrFds = currentProcess_->fds().pipe2(0);
        int ret = errnoOrFds.errorOrWith<int>([&](std::pair<FD, FD> fds) {
            std::vector<u32> fdsbuf {{ (u32)fds.first.fd, (u32)fds.second.fd }};
            mem::Ptr ptr { pipefd.address() };
            mmu_->writeToMmu(ptr, fdsbuf);
            return 0;
        });
        if(kernel_.logSyscalls()) {
            print("Sys::pipe(pipefd={:#x}) = {}", pipefd.address(), ret);
        }
        return ret;
    }

    int Sys::dup(int oldfd) {
        FD newfd = currentProcess_->fds().dup(FD{oldfd});
        if(kernel_.logSyscalls()) print("Sys::dup(oldfd={}) = {}", oldfd, newfd.fd);
        return newfd.fd;
    }

    int Sys::dup2(int oldfd, int newfd) {
        FD fd = currentProcess_->fds().dup2(FD{oldfd}, FD{newfd});
        if(kernel_.logSyscalls()) print("Sys::dup2(oldfd={}, newfd={}) = {}", oldfd, newfd, fd.fd);
        return fd.fd;
    }

    int Sys::setitimer(int which, const mem::Ptr new_value, mem::Ptr old_value) {
        if(kernel_.logSyscalls()) {
            print("Sys::setitimer(which={}, new_value={:#x}, old_value={:#x}) = {}",
                                    which, new_value.address(), old_value.address(), -ENOTSUP);
        }
        warn("setitimer not implemented");
        return -ENOTSUP;
    }

    int Sys::getpid() {
        verify(!!currentThread_);
        int pid = currentThread_->description().pid;
        if(kernel_.logSyscalls()) print("Sys::getpid() = {}", pid);
        return pid;
    }

    int Sys::select(int nfds, mem::Ptr readfds, mem::Ptr writefds, mem::Ptr exceptfds, mem::Ptr timeout) {
        static_assert(sizeof(FS::SelectData::readfds) == sizeof(fd_set));
        FS::SelectData selectData;
        selectData.fds.reserve(nfds);
        for(int fd = 0; fd < nfds; ++fd) {
            selectData.fds.push_back(currentProcess_->fds()[fd]);
        }
        if(!!readfds) mmu_->copyFromMmu((u8*)&selectData.readfds, readfds, sizeof(selectData.readfds));
        if(!!writefds) mmu_->copyFromMmu((u8*)&selectData.writefds, writefds, sizeof(selectData.writefds));
        if(!!exceptfds) mmu_->copyFromMmu((u8*)&selectData.exceptfds, exceptfds, sizeof(selectData.exceptfds));
        Timer* timer = kernel_.timers().getOrTryCreate(0);
        auto timeoutDuration = timer->readTimeval(*mmu_, timeout);
        if(!!timeoutDuration && timeoutDuration->seconds == 0 && timeoutDuration->nanoseconds == 0) {
            int ret = kernel_.fs().selectImmediate(&selectData);
            if(kernel_.logSyscalls()) {
                print("Sys::select(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x}) = {}",
                            nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), ret);
            }
            if(ret < 0) return ret;
            if(!!readfds) mmu_->copyToMmu(readfds, (const u8*)&selectData.readfds, sizeof(selectData.readfds));
            if(!!writefds) mmu_->copyToMmu(writefds, (const u8*)&selectData.writefds, sizeof(selectData.writefds));
            if(!!exceptfds) mmu_->copyToMmu(exceptfds, (const u8*)&selectData.exceptfds, sizeof(selectData.exceptfds));
            return ret;
        } else {
            kernel_.scheduler().select(currentThread_, nfds, readfds, writefds, exceptfds, timeout);
            return 0;
        }
    }

    int Sys::sched_yield() {
        if(kernel_.logSyscalls()) print("Sys::sched_yield()");
        verify(!!currentThread_);
        currentThread_->yield();
        return 0;
    }

    mem::Ptr Sys::mremap(mem::Ptr old_address, size_t old_size, size_t new_size, int flags, mem::Ptr new_address) {
        if(kernel_.logSyscalls()) {
            print("Sys::mremap(old_address={:#x}, old_size={}, new_size={}, flags={}, new_address={:#x}) = {}",
                                    old_address.address(), old_size, new_size, flags, new_address.address(), -ENOTSUP);
        }
        warn("mremap not implemented");
        return mem::Ptr{(u64)-ENOTSUP};
    }

    int Sys::msync(mem::Ptr addr, size_t length, int flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::msync(addr={:#x}, length={:#x}, flags={:#x}) = {}",
                                    addr.address(), length, flags, -ENOTSUP);
        }
        warn("msync not implemented");
        return -ENOTSUP;
    }

    int Sys::mincore(mem::Ptr addr, size_t length, mem::Ptr8 vec) {
        auto res = mmu_->mincore(addr.address(), length);
        mmu_->copyToMmu(vec, res.data(), res.size());
        if(kernel_.logSyscalls()) {
            print("Sys::mincore(addr={:#x}, length={:#x}, vec={:#x}) = {}",
                                    addr.address(), length, vec.address(), 0);
        }
        return 0;
    }

    int Sys::madvise(mem::Ptr addr, size_t length, int advice) {
        if(Host::Madvise::isDontNeed(advice)) {
            if(kernel_.logSyscalls()) {
                print("Sys::madvise(addr={:#x}, length={}, advice=DONT_NEED) = {}",
                                        addr.address(), length, advice, 0);
            }
            return 0;
        } else if(Host::Madvise::isFree(advice)) {
            if(kernel_.logSyscalls()) {
                print("Sys::madvise(addr={:#x}, length={}, advice=FREE) = {}",
                                        addr.address(), length, advice, 0);
            }
            return 0;
        } else {
            int ret = 0;
            if(kernel_.logSyscalls()) {
                print("Sys::madvise(addr={:#x}, length={}, advice={}) = {}",
                                        addr.address(), length, advice, ret);
            }
            warn("madvise not implemented with advice {} - returning bogus 0", advice);
            return ret;
        }
    }

    int Sys::shmget(key_t key, size_t size, int shmflg) {
        int ret = -ENOTSUP;
        if(kernel_.isShmEnabled()) {
            bool isIpcPrivate = Host::ShmGet::isIpcPrivate(key);
            int mode = Host::ShmGet::getModePermissions(shmflg);
            bool isIpcCreate = Host::ShmGet::isIpcCreate(shmflg);
            bool isIpcExcl = Host::ShmGet::isIpcExcl(shmflg);

            BitFlags<SharedMemory::GetFlags> flags;
            if(isIpcCreate) flags.add(SharedMemory::GetFlags::CREATE);
            if(isIpcExcl) flags.add(SharedMemory::GetFlags::EXCL);

            auto errnoOrId = kernel_.shm().get(
                isIpcPrivate ? SharedMemory::IPC_PRIVATE : SharedMemory::Key{key},
                size,
                mode,
                flags);

            ret = errnoOrId.errorOrWith<int>([&](const SharedMemory::Id& id) {
                return id.value;
            });
        }
        if(kernel_.logSyscalls()) {
            print("Sys::shmget(key={}, size={:#x}, shmflg={:#x}) = {}", key, size, shmflg, ret);
        }
        return ret;
    }

    mem::Ptr Sys::shmat(int shmid, mem::Ptr shmaddr, int shmflg) {
        u64 ret = (u64)-ENOTSUP;
        if(kernel_.isShmEnabled()) {
            BitFlags<SharedMemory::AtFlags> flags;
            if(Host::ShmAt::isReadOnly(shmflg)) flags.add(SharedMemory::AtFlags::READ_ONLY);
            if(Host::ShmAt::isExecute(shmflg)) flags.add(SharedMemory::AtFlags::EXEC);
            if(Host::ShmAt::isRemap(shmflg)) flags.add(SharedMemory::AtFlags::REMAP);
            auto errnoOrAddr = kernel_.shm().attach(mmu_, SharedMemory::Id{shmid}, shmaddr.address(), flags);
            ret = errnoOrAddr.errorOrWith<u64>([&](u64 addr) {
                return addr;
            });
        }
        if(kernel_.logSyscalls()) {
            print("Sys::shmat(shmid={}, shmaddr={:#x}, shmflg={:#x}) = {}", shmid, shmaddr.address(), shmflg, ret);
        }
        return mem::Ptr{ret};
    }

    int Sys::shmctl(int shmid, int cmd, mem::Ptr buf) {
        int ret = -ENOTSUP;
        if(kernel_.isShmEnabled()) {
            if(Host::ShmCtl::isRmid(cmd)) {
                ret = kernel_.shm().rmid(SharedMemory::Id{shmid});
            }
        }
        if(kernel_.logSyscalls()) {
            print("Sys::shmctl(shmid={}, cmd={:#x}, buf={:#x}) = {}", shmid, cmd, buf.address(), ret);
        }
        return ret;
    }

    int Sys::socket(int domain, int type, int protocol) {
        FD fd = currentProcess_->fds().socket(domain, type, protocol);
        if(kernel_.logSyscalls()) {
            print("Sys::socket(domain={}, type={}, protocol={}) = {}",
                                    domain, type, protocol, fd.fd);
        }
        return fd.fd;
    }

    int Sys::connect(int sockfd, mem::Ptr addr, size_t addrlen) {
        Buffer buffer(addrlen, 0x0);
        mmu_->copyFromMmu(buffer.data(), addr, buffer.size());
        auto descriptor = currentProcess_->fds()[sockfd];
        int ret = kernel_.fs().connect(descriptor, buffer);
        if(kernel_.logSyscalls()) {
            print("Sys::connect(sockfd={}, addr={:#x}, addrlen={}) = {}",
                        sockfd, addr.address(), addrlen, ret);
        }
        return ret;
    }

    ssize_t Sys::sendto(int sockfd, mem::Ptr buf, size_t len, int flags, mem::Ptr dest_addr, socklen_t addrlen) {
        verify(dest_addr.address() == 0);
        verify(addrlen == 0);
        Buffer buffer(len, 0x0);
        mmu_->copyFromMmu(buffer.data(), buf, buffer.size());
        auto descriptor = currentProcess_->fds()[sockfd];
        ssize_t ret = kernel_.fs().send(descriptor, buffer, flags);
        if(kernel_.logSyscalls()) {
            print("Sys::sendto(sockfd={}, buf={:#x}, len={}, flags={}, dest_addr={:#x}, addrlen={}) = {}",
                        sockfd, buf.address(), len, flags, dest_addr.address(), addrlen, ret);
        }
        return ret;
    }

    int Sys::getsockname(int sockfd, mem::Ptr addr, mem::Ptr32 addrlen) {
        u32 buffersize = mmu_->read32(addrlen);
        auto descriptor = currentProcess_->fds()[sockfd];
        ErrnoOrBuffer sockname = kernel_.fs().getsockname(descriptor, buffersize);
        if(kernel_.logSyscalls()) {
            print("Sys::getsockname(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen.address(), sockname.errorOr(0));
            // sockname.errorOrWith<int>([&](const auto& buffer) {
            //     print("{:p}", (const char*)buffer.data());
            //     return 0;
            // });
        }
        return sockname.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(addr, buffer.data(), buffer.size());
            mmu_->write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::getpeername(int sockfd, mem::Ptr addr, mem::Ptr32 addrlen) {
        u32 buffersize = mmu_->read32(addrlen);
        auto descriptor = currentProcess_->fds()[sockfd];
        ErrnoOrBuffer peername = kernel_.fs().getpeername(descriptor, buffersize);
        if(kernel_.logSyscalls()) {
            print("Sys::getpeername(sockfd={}, addr={:#x}, addrlen={:#x}) = {}",
                        sockfd, addr.address(), addrlen.address(), peername.errorOr(0));
            // peername.errorOrWith<int>([&](const auto& buffer) {
            //     print("{}", (const char*)buffer.data());
            //     return 0;
            // });
        }
        return peername.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(addr, buffer.data(), buffer.size());
            mmu_->write32(addrlen, (u32)buffer.size());
            return 0;
        });
    }

    int Sys::socketpair(int domain, int type, int protocol, mem::Ptr32 sv) {
        if(kernel_.logSyscalls()) {
            std::vector<int> svs = mmu_->readFromMmu<int>(mem::Ptr8{sv.address()}, 2);
            print("Sys::socketpair(domain={}, type={}, protocol={}, sv=[{},{}]) = {}",
                domain, type, protocol, svs[0], svs[1], -ENOTSUP);
        }
        warn("socketpair not implemented");
        return -ENOTSUP;
    }

    int Sys::setsockopt(int sockfd, int level, int optname, mem::Ptr optval, socklen_t optlen) {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        verify(!!optval, "getsockopt with null optval not implemented");
        Buffer buf((size_t)optlen, 0x0);
        mmu_->copyFromMmu(buf.data(), optval, buf.size());
        auto descriptor = currentProcess_->fds()[sockfd];
        int ret = kernel_.fs().setsockopt(descriptor, level, optname, buf);
        if(kernel_.logSyscalls()) {
            print("Sys::setsockopt(sockfd={}, level={}, optname={}, optval={:#x}, optlen={}) = {}",
                        sockfd, level, optname, optval.address(), optlen, ret);
        }
        return ret;
    }
    
    int Sys::getsockopt(int sockfd, int level, int optname, mem::Ptr optval, mem::Ptr32 optlen) {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        verify(!!optval, "getsockopt with null optval not implemented");
        verify(!!optlen, "getsockopt with null optlen not implemented");
        u32 len = mmu_->read32(optlen);
        Buffer buf(len, 0x0);
        mmu_->copyFromMmu(buf.data(), optval, buf.size());
        auto descriptor = currentProcess_->fds()[sockfd];
        ErrnoOrBuffer errnoOrBuffer = kernel_.fs().getsockopt(descriptor, level, optname, buf);
        int ret = errnoOrBuffer.errorOrWith<int>([&](const Buffer& buffer) {
            mmu_->copyToMmu(optval, buffer.data(), buffer.size());
            mmu_->write32(optlen, (u32)buffer.size());
            return 0;
        });
        if(kernel_.logSyscalls()) {
            print("Sys::getsockopt(sockfd={}, level={}, optname={}, optval={:#x}, optlen={:#x}) = {}",
                        sockfd, level, optname, optval.address(), optlen.address(), ret);
        }
        return ret;
    }

    [[nodiscard]] static bool checkCloneFlags(const Host::CloneFlags& flags) {
        bool expected =
                true
                && flags.childClearTid
                && !flags.childSetTid
                && !flags.clearSignalHandlers
                && flags.cloneSignalHandlers
                && flags.cloneFiles
                && flags.cloneFs
                && !flags.cloneIo
                && !flags.cloneParent
                && flags.parentSetTid
                && !flags.clonePidFd
                && flags.setTls
                && flags.cloneThread
                && flags.cloneVm
                && !flags.cloneVfork;
        if(!expected) {
            if(!flags.childClearTid) puts("Expected cloneFlags.childClearTid == true");
            if(!!flags.childSetTid) puts("Expected cloneFlags.childSetTid == false");
            if(!!flags.clearSignalHandlers) puts("Expected cloneFlags.clearSignalHandlers == false");
            if(!flags.cloneSignalHandlers) puts("Expected cloneFlags.cloneSignalHandlers == true");
            if(!flags.cloneFiles) puts("Expected cloneFlags.cloneFiles == true");
            if(!flags.cloneFs) puts("Expected cloneFlags.cloneFs == true");
            if(!!flags.cloneIo) puts("Expected cloneFlags.cloneIo == false");
            if(!!flags.cloneParent) puts("Expected cloneFlags.cloneParent == false");
            if(!flags.parentSetTid) puts("Expected cloneFlags.parentSetTid == true");
            if(!!flags.clonePidFd) puts("Expected cloneFlags.clonePidFd == false");
            if(!flags.setTls) puts("Expected cloneFlags.setTls == true");
            if(!flags.cloneThread) puts("Expected cloneFlags.cloneThread == true");
            if(!flags.cloneVm) puts("Expected cloneFlags.cloneVm == true");
            if(!!flags.cloneVfork) puts("Expected cloneFlags.cloneVfork == false");
            return false;
        }
        return true;
    }

    [[nodiscard]] static bool checkCloneFlagsFork(const Host::CloneFlags& flags) {
        bool expected =
                true
                && !flags.clearSignalHandlers
                && !flags.cloneSignalHandlers
                && !flags.cloneFiles
                && !flags.cloneFs
                && !flags.cloneIo
                && !flags.cloneParent
                && !flags.parentSetTid
                && !flags.clonePidFd
                && !flags.setTls
                && !flags.cloneThread;
        if(!expected) {
            if(!!flags.clearSignalHandlers) puts("Expected cloneFlags.clearSignalHandlers == false");
            if(!!flags.cloneSignalHandlers) puts("Expected cloneFlags.cloneSignalHandlers == false");
            if(!!flags.cloneFiles) puts("Expected cloneFlags.cloneFiles == false");
            if(!!flags.cloneFs) puts("Expected cloneFlags.cloneFs == false");
            if(!!flags.cloneIo) puts("Expected cloneFlags.cloneIo == false");
            if(!!flags.cloneParent) puts("Expected cloneFlags.cloneParent == false");
            if(!!flags.parentSetTid) puts("Expected cloneFlags.parentSetTid == false");
            if(!!flags.clonePidFd) puts("Expected cloneFlags.clonePidFd == false");
            if(!!flags.setTls) puts("Expected cloneFlags.setTls == false");
            if(flags.cloneThread) puts("Expected cloneFlags.cloneThread == false");
            return false;
        }
        return true;
    }

    long Sys::clone(unsigned long flags, mem::Ptr stack, mem::Ptr32 parent_tid, mem::Ptr32 child_tid, unsigned long tls) {
        Host::CloneFlags cloneFlags = Host::fromCloneFlags(flags);
        Thread* newThread { nullptr };
        if(cloneFlags.cloneThread) {
            verify(cloneFlags.cloneSignalHandlers, "cloneThread implies cloneSignalHandlers");
            verify(cloneFlags.cloneVm, "cloneThread implies cloneVm");
            verify(!!currentProcess_);
            verify(!!currentThread_);
            verify(checkCloneFlags(cloneFlags));

            newThread = currentProcess_->addThread(kernel_.processTable());
        } else {
            verify(!!currentProcess_);
            verify(!!currentThread_);
            verify(!cloneFlags.cloneFiles, "cloneFiles without cloneThread not supported");
            BitFlags<Process::CloneFlags> cflags;
            if(cloneFlags.cloneVm) cflags.add(Process::CloneFlags::VM);
            Process* newProcess = [&]() -> Process* {
                auto process = currentProcess_->clone(kernel_.processTable(), cflags);
                verify(!!process, "Unable to create new process");
                return kernel_.processTable().addProcess(std::move(process));
            }();
            bool flagsOk = checkCloneFlagsFork(cloneFlags);
            if(!flagsOk) {
                if(kernel_.logSyscalls()) {
                    print("Sys::clone(flags={}, stack={:#x}, parent_tid={:#x}, child_tid={:#x}, tls={}) = {}",
                                flags, stack.address(), parent_tid.address(), child_tid.address(), tls, -ENOTSUP);
                }
                return -ENOTSUP;
            }
            newThread = newProcess->addThread(kernel_.processTable());
            if(cloneFlags.cloneVfork) {
                kernel_.scheduler().suspendUntilVmReleased(currentThread_);
            }
        }


        verify(!!newThread);
        mem::Mmu childMmu(newThread->process()->addressSpace());
        newThread->cloneState(*currentThread_);
        newThread->setSyscallOutput(0);
        if(stack.address() != 0) { // using nullptr for stack means keeping the same stack
            newThread->setStackPtr(stack.address());
        } else {
            verify(!cloneFlags.cloneVm, "Danger, copying stack in child in same addresspace");
        }
        childMmu.setRegionName(stack.address(), fmt::format("Stack of thread {}", newThread->description().tid));
        if(cloneFlags.setTls) {
            newThread->setTlsBase(tls);
        }
        if(cloneFlags.childClearTid) {
            newThread->setClearChildTid(child_tid);
        }
        if(!!child_tid && cloneFlags.childSetTid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            childMmu.write32(child_tid, (u32)newThread->description().tid);
        }
        if(!!parent_tid && cloneFlags.parentSetTid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_->write32(parent_tid, (u32)newThread->description().tid);
        }
        kernel_.scheduler().addThread(newThread);
        long ret = newThread->description().tid;
        if(kernel_.logSyscalls()) {
            print("Sys::clone(flags={}, stack={:#x}, parent_tid={:#x}, child_tid={:#x}, tls={}) = {}",
                        flags, stack.address(), parent_tid.address(), child_tid.address(), tls, ret);
        }
        return ret;
    }

    int Sys::fork() {
        if(!kernel_.isForkEnabled()) {
            warn("fork disabled");
            return -ENOTSUP;
        }
        warn("Sys::fork => Sys::clone");
        return (int)clone(CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID, mem::Ptr::null(), mem::Ptr32::null(), mem::Ptr32::null(), 0);
    }

    int Sys::vfork() {
        if(!kernel_.isForkEnabled()) {
            warn("vfork disabled");
            return -ENOTSUP;
        }
        warn("Sys::vfork => Sys::fork");
        return fork();
    }

    int Sys::execve(mem::Ptr pathname, mem::Ptr64 argv, mem::Ptr64 envp) {
        verify(!!pathname, "cannot exec with null pathname");
        verify(!!argv, "cannot exec with null argv");
        std::string path = mmu_->readString(pathname);
        std::vector<std::string> args;
        while(true) {
            u64 arg = mmu_->read64(argv);
            if(arg == 0) break;
            args.push_back(mmu_->readString(mem::Ptr{arg}));
            ++argv;
        }
        verify(!args.empty(), "unexpected empty argv list in exec");
        args.erase(args.begin()); // args[0] is dropped
        std::vector<std::string> envs;
        if(!!envp) {
            while(true) {
                u64 env = mmu_->read64(envp);
                if(env == 0) break;
                envs.push_back(mmu_->readString(mem::Ptr{env}));
                ++envp;
            }
        } else {
            warn("calling exec with null envp");
        }

        int ret = 0;

        {
            ExecVE execve(kernel_.processTable(), *currentProcess_, kernel_.scheduler(), kernel_.fs());
            ErrnoOr<Thread*> errnoOrThread = execve.exec(path, args, envs);
            ret = errnoOrThread.errorOr(0);
            errnoOrThread.with([&](Thread* newThread) {
                verify(!!newThread, "execve failed");
                currentThread_ = newThread;
            });
        }

        if(kernel_.logSyscalls()) {
            auto argvstring = fmt::format("{}", fmt::join(args, ", "));
            auto envpstring = fmt::format("{}", fmt::join(envs, ", "));
            print("Sys::exec(pathname={}, argv={}, envp={}) = {}",
                        path, argvstring, envpstring, ret);
        }
        return ret;
    }

    int Sys::exit(int status) {
        if(kernel_.logSyscalls()) {
            print("Sys::exit(status={})", status);
        }
        kernel_.scheduler().terminate(currentThread_, status);
        return status;
    }

    int Sys::wait4(pid_t pid, mem::Ptr32 wstatus, int options, mem::Ptr rusage) {
        Host::WaitOptions opt = Host::fromWaitOptions(options);
        if(opt.continued) warn("continued option unsupported in wait4");
        verify(!rusage, "non-null rusage unsupported in wait4");
        if(kernel_.logSyscalls()) {
            print("Sys::wait4(pid={}, wstatus={:#x}, options={}, rusage={:#x}) = {}", pid, wstatus.address(), options, rusage.address(), 0);
        }
        if(currentProcess_->nbChildren() == 0) {
            return -ECHILD;
        } else if(opt.untraced) {
            auto child = currentProcess_->tryRetrieveExitedChild();
            if(child) {
                return child->pid;
            }
        } else if(opt.nohang && currentProcess_->nbExitedChildren() == 0) {
            return 0;
        }
        kernel_.scheduler().wait4(currentThread_, (int)pid, wstatus);
        return 0;
    }

    int Sys::kill(pid_t pid, int sig) {
        if(kernel_.logSyscalls()) {
            print("Sys::kill(pid={}, sig={}) = {}", pid, sig, -ENOTSUP);
        }
        warn("kill not implemented");
        return -ENOTSUP;
    }

    int Sys::uname(mem::Ptr buf) {
        ErrnoOrBuffer errnoOrBuffer = Host::uname();
        if(kernel_.logSyscalls()) {
            print("Sys::uname(buf={:#x}) = {}",
                        buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::shmdt(mem::Ptr shmaddr) {
        if(!kernel_.isShmEnabled()) return -ENOTSUP;
        int ret = kernel_.shm().detach(mmu_, shmaddr.address());
        if(kernel_.logSyscalls()) {
            print("Sys::shmdt({:#x}) = {}", shmaddr.address(), ret);
        }
        return ret;
    }

    int Sys::fcntl(int fd, int cmd, int arg) {
        auto command = Host::Fcntl::toCommand(cmd);
        if(command) {
            int ret = currentProcess_->fds().fcntl(FD{fd}, *command, arg);
            if(kernel_.logSyscalls()) {
                print("Sys::fcntl(fd={}, cmd={}, arg={}) = {}", fd, toString(*command), arg, ret);
            }
            return ret;
        } else {
            warn("fcntl(cmd={}) not supported", cmd);
            if(kernel_.logSyscalls()) {
                print("Sys::fcntl(fd={}, cmd={}, arg={}) = {}", fd, cmd, arg, -ENOTSUP);
            }
            return -ENOTSUP;
        }
    }

    int Sys::flock(int fd, int operation) {
        auto descriptor = currentProcess_->fds()[fd];
        int ret = kernel_.fs().flock(descriptor, operation);
        if(kernel_.logSyscalls()) print("Sys::flock(fd={}, operation={}) = {}", fd, operation, ret);
        return ret;
    }

    int Sys::fsync(int fd) {
        if(kernel_.logSyscalls()) print("Sys::fsync(fd={}) = {}", fd, -ENOTSUP);
        warn("fsync not implemented");
        return -ENOTSUP;
    }

    int Sys::fdatasync(int fd) {
        if(kernel_.logSyscalls()) print("Sys::fdatasync(fd={}) = {}", fd, -ENOTSUP);
        warn("fdatasync not implemented");
        return -ENOTSUP;
    }

    int Sys::truncate(mem::Ptr8 path_, off_t length) {
        auto pathname = mmu_->readString(path_);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), pathname);
        int ret = [&]() {
            if(!path) return -ENOENT;
            return kernel_.fs().truncate(*path, length);
        }();
        if(kernel_.logSyscalls()) print("Sys::truncate(path={}, length={}) = {}", pathname, length, ret);
        return ret;
    }

    int Sys::ftruncate(int fd, off_t length) {
        auto descriptor = currentProcess_->fds()[fd];
        int ret = kernel_.fs().ftruncate(descriptor, length);
        if(kernel_.logSyscalls()) print("Sys::ftruncate(fd={}, length={}) = {}", fd, length, ret);
        return ret;
    }

    int Sys::getcwd(mem::Ptr buf, size_t size) {
        ErrnoOrBuffer errnoOrBuffer = Host::getcwd(size);
        if(kernel_.logSyscalls()) {
            print("Sys::getcwd(buf={:#x}, size={}) = {:#x}",
                        buf.address(), size, errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
                            return (int)buffer.size();
                        }));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (int)buffer.size();
        });
    }

    int Sys::chdir(mem::Ptr pathname) {
        auto newpath = mmu_->readString(pathname);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), newpath);
        int ret = 0;
        if(!path) {
            ret = -ENOENT;
        } else {
            auto* cwd = currentProcess_->chdir(*path);
            ret = !!cwd ? 0 : -ENOENT;
        }
        if(kernel_.logSyscalls()) {
            print("Sys::chdir(path={}) = {}", newpath, ret);
        }
        return ret;
    }

    int Sys::rename(mem::Ptr oldpathname, mem::Ptr newpathname) {
        auto oldname = mmu_->readString(oldpathname);
        auto newname = mmu_->readString(newpathname);
        auto oldpath = kernel_.fs().resolvePath(currentProcess_->cwd(), oldname);
        auto newpath = kernel_.fs().resolvePath(currentProcess_->cwd(), newname);
        
        int ret = [&]() {
            if(!oldpath) return -ENOENT;
            if(!newpath) return -ENOENT;
            return kernel_.fs().rename(*oldpath, *newpath);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::rename(oldpath={}, newpath={}) = {}", oldname, newname, ret);
        }
        return ret;
    }

    int Sys::mkdir(mem::Ptr pathname, mode_t mode) {
        auto pathname_ = mmu_->readString(pathname);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), pathname_);
        auto ret = [&]() {
            if(!path) return -ENOENT;
            return kernel_.fs().mkdir(*path);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::mkdir(path={}, mode={:o}) = {}", pathname_, mode, ret);
        }
        return ret;
    }

    int Sys::unlink([[maybe_unused]] mem::Ptr pathname) {
        auto pathname_ = mmu_->readString(pathname);
        auto path = kernel_.fs().resolvePath(currentProcess_->cwd(), pathname_);
        int ret = [&]() {
            if(!path) return -ENOENT;
            return kernel_.fs().unlink(*path);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::unlink(path={}) = {}", pathname_, ret);
        }
        return ret;
    }

    ssize_t Sys::readlink(mem::Ptr pathname, mem::Ptr buf, size_t bufsiz) {
        std::string path = mmu_->readString(pathname);
        auto linkpath = kernel_.fs().resolvePath(currentProcess_->cwd(), path);
        auto errnoOrBuffer = [&]() {
            if(!linkpath) return ErrnoOrBuffer(-ENOENT);
            return kernel_.fs().readlink(*linkpath, bufsiz);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::readlink(path={}, buf={:#x}, size={}) = {:#x}",
                        path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                            std::string link((const char*)buffer.data(), buffer.size());
                            fmt::print("  link={}", link);
                            return (ssize_t)buffer.size();
                        }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::chmod(mem::Ptr pathname, mode_t mode) {
        if(kernel_.logSyscalls()) {
            std::string path = mmu_->readString(pathname);
            print("Sys::chmod(path={}, mode={}) = {}",
                        path, mode, -ENOTSUP);
        }
        warn("chmod not implemented");
        return -ENOTSUP;
    }

    int Sys::fchmod(int fd, mode_t mode) {
        if(kernel_.logSyscalls()) {
            print("Sys::fchmod(fd={}, mode={}) = {}", fd, mode, -ENOTSUP);
        }
        warn("fchmod not implemented");
        return -ENOTSUP;
    }

    int Sys::chown(mem::Ptr pathname, uid_t owner, gid_t group) {
        if(kernel_.logSyscalls()) {
            std::string path = mmu_->readString(pathname);
            print("Sys::chown(path={}, owner={}, group={}) = {}",
                        path, owner, group, -ENOTSUP);
        }
        warn("chown not implemented");
        return -ENOTSUP;
    }

    int Sys::fchown(int fd, uid_t owner, gid_t group) {
        if(kernel_.logSyscalls()) {
            print("Sys::fchown(fd={}, owner={}, group={}) = {}",
                        fd, owner, group, -ENOTSUP);
        }
        warn("chown not implemented");
        return -ENOTSUP;
    }

    int Sys::umask(int mask) {
        if(kernel_.logSyscalls()) {
            print("Sys::umask(mask={}) = {}",
                        mask, 0777);
        }
        warn("umask not implemented");
        return 0777;
    }

    int Sys::gettimeofday(mem::Ptr tv, mem::Ptr tz) {
        PreciseTime time = kernel_.scheduler().kernelTime();
        if(kernel_.logSyscalls()) {
            print("Sys::gettimeofday(tv={:#x}, tz={:#x}) = {:#x}",
                        tv.address(), tz.address(), 0);
        }
        if(!!tv) {
            auto timevalBuffer = Host::gettimeofday(time);
            mmu_->copyToMmu(tv, timevalBuffer.data(), timevalBuffer.size());
        }
        if(!!tz) {
            auto timezoneBuffer = Host::gettimezone();
            mmu_->copyToMmu(tv, timezoneBuffer.data(), timezoneBuffer.size());
        }
        return 0;
    }

    int Sys::getrusage(int who, mem::Ptr usage) {
        if(kernel_.logSyscalls()) {
            print("Sys::getrusage(who={}, usage={:#x}) = {}",
                        who, usage.address(), -ENOTSUP);
        }
        warn("getrusage not implemented");
        return -ENOTSUP;
    }

    int Sys::sysinfo(mem::Ptr info) {
        auto errnoOrBuffer = Host::sysinfo();
        if(kernel_.logSyscalls()) {
            print("Sys::sysinfo(info={:#x}) = {}",
                        info.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(info, buffer.data(), buffer.size());
            return 0;
        });
    }

    clock_t Sys::times(mem::Ptr buf) {
        if(kernel_.logSyscalls()) {
            print("Sys::times(buf={:#x}) = {}",
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

    int Sys::setpgid(pid_t pid, pid_t pgid) {
        verify(!!currentThread_);
        if(pid == 0 || pid == currentThread_->description().pid) {
            Process* other = kernel_.processTable().findByPid(pgid);
            if(!other) return -ESRCH;
            if(other->sid() != currentProcess_->sid()) return -EPERM;
            currentProcess_->setpgid(pgid);
            return 0;
        }
        if(Process* child = currentProcess_->tryGetChild(pid)) {
            warn("setpgid of child not implemented");
            (void)child;
        }
        if(kernel_.logSyscalls()) {
            print("Sys::setpgid(pid={}, pgid={}) = {}", pid, pgid, 0);
        }
        return 0;
    }

    int Sys::getppid() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getppid();
    }

    int Sys::getpgrp() { // NOLINT(readability-convert-member-functions-to-static)
        return Host::getpgrp();
    }

    int Sys::getgroups(int size, mem::Ptr list) {
        ErrnoOrBuffer groups = Host::getgroups(size);
        int ret = groups.errorOrWith<int>([&](const Buffer& buf) {
            if(size > 0) {
                mmu_->copyToMmu(list, buf.data(), buf.size());
            }
            return (int)(buf.size() / sizeof(gid_t));
        });
        if(kernel_.logSyscalls()) {
            print("Sys::getgroups(size={}, list={:#x}) = {}", size, list.address(), ret);
        }
        return ret;
    }

    int Sys::getresuid(mem::Ptr32 ruid, mem::Ptr32 euid, mem::Ptr32 suid) {
        Host::UserCredentials creds = Host::getUserCredentials();
        mmu_->write32(ruid, (u32)creds.ruid);
        mmu_->write32(euid, (u32)creds.euid);
        mmu_->write32(suid, (u32)creds.suid);
        return 0;
    }

    int Sys::getresgid(mem::Ptr32 rgid, mem::Ptr32 egid, mem::Ptr32 sgid) {
        Host::UserCredentials creds = Host::getUserCredentials();
        mmu_->write32(rgid, (u32)creds.rgid);
        mmu_->write32(egid, (u32)creds.egid);
        mmu_->write32(sgid, (u32)creds.sgid);
        return 0;
    }

    pid_t Sys::getpgid(pid_t pid) {
        verify(!!currentThread_);
        verify(pid == 0 || pid == currentThread_->description().pid, "getpgid with nonzero or non-process pid not supported");
        int pgid = currentThread_->description().pgid;
        if(kernel_.logSyscalls()) {
            print("Sys::getpgid(pid={}) = {})", pid, pgid);
        }
        return pgid;
    }

    int Sys::rt_sigtimedwait(mem::Ptr set, mem::Ptr info, mem::Ptr timeout) {
        if(kernel_.logSyscalls()) {
            print("Sys::rt_sigtimedwait(set={:#x}, info={:#x}, timeout={:#x}) = {})", set.address(), info.address(), timeout.address(), -ENOTSUP);
        }
        warn("rt_sigtimedwait not implemented");
        return -ENOTSUP;
    }

    int Sys::sigaltstack(mem::Ptr ss, mem::Ptr old_ss) {
        if(kernel_.logSyscalls()) {
            print("Sys::sigaltstack(ss={:#x}, old_ss={:#x}) = {}", ss.address(), old_ss.address(), -ENOTSUP);
        }
        warn("sigaltstack not implemented");
        return -ENOTSUP;
    }

    int Sys::utime(mem::Ptr filename, mem::Ptr times) {
        if(kernel_.logSyscalls()) {
            std::string path = mmu_->readString(filename);
            print("Sys::utime(filename={}, times={:#x} = {})", path, times.address(), -ENOTSUP);
        }
        warn("utime not implemented");
        return -ENOTSUP;
    }

    int Sys::statfs(mem::Ptr pathname, mem::Ptr buf) {
        std::string path = mmu_->readString(pathname);
        auto errnoOrBuffer = Host::statfs(path);
        if(kernel_.logSyscalls()) {
            print("Sys::statfs(pathname={}, buf={:#x} = {})", path, buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::fstatfs(int fd, mem::Ptr buf) {
        auto descriptor = currentProcess_->fds()[fd];
        auto errnoOrBuffer = kernel_.fs().fstatfs(descriptor);
        if(kernel_.logSyscalls()) {
            print("Sys::fstatfs(fd={}, buf={:#x} = {})", fd, buf.address(), errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::getpriority(int which, id_t who) {
        if(kernel_.logSyscalls()) {
            print("Sys::getpriority(which={}, who={}) = {}", which, who, -ENOTSUP);
        }
        warn("getpriority not implemented");
        return -ENOTSUP;
    }

    int Sys::setpriority(int which, id_t who, int prio) {
        if(kernel_.logSyscalls()) {
            print("Sys::setpriority(which={}, who={}, prio={}) = {}", which, who, prio, -ENOTSUP);
        }
        warn("setpriority not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_getparam(pid_t pid, mem::Ptr param) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_getparam(pid={}, param={:#x}) = {}", pid, param.address(), -ENOTSUP);
        }
        warn("sched_getparam not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_setscheduler(pid_t pid, int policy, mem::Ptr param) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_setscheduler(pid={}, policy={}, param={:#x}) = {}", pid, policy, param.address(), -ENOTSUP);
        }
        warn("sched_setscheduler not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_getscheduler(pid_t pid) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_getscheduler(pid={}) = {}", pid, -ENOTSUP);
        }
        warn("sched_getscheduler not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_get_priority_max(int policy) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_get_priority_max(policy={}) = {}", policy, -ENOTSUP);
        }
        warn("sched_get_priority_max not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_get_priority_min(int policy) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_get_priority_min(policy={}) = {}", policy, -ENOTSUP);
        }
        warn("sched_get_priority_min not implemented");
        return -ENOTSUP;
    }

    int Sys::mlock(mem::Ptr addr, size_t len) {
        if(kernel_.logSyscalls()) {
            print("Sys::mlock(addr={:#x}, len={}) = {}", addr.address(), len, 0);
        }
        return 0;
    }

    int Sys::munlock(mem::Ptr addr, size_t len) {
        if(kernel_.logSyscalls()) {
            print("Sys::munlock(addr={:#x}, len={}) = {}", addr.address(), len, 0);
        }
        return 0;
    }

    int Sys::mlockall(int flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::mlockall(flags={:#x}) = {}", flags, 0);
        }
        return 0;
    }

    int Sys::munlockall(int flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::munlockall(flags={:#x}) = {}", flags, 0);
        }
        return 0;
    }

    u64 Sys::exit_group(int status) {
        if(kernel_.logSyscalls()) print("Sys::exit_group(status={})", status);
        kernel_.scheduler().terminateGroup(currentProcess_, status);
        currentProcess_->notifyExit(status, {});
        return (u64)status;
    }

    struct [[gnu::packed]] EpollEvent {
        u32 event;
        u64 data;
    };

    int Sys::epoll_wait(int epfd, mem::Ptr events, int maxevents, int timeout) {
        if(!events) return -EFAULT;
        if(maxevents <= 0) return -EINVAL;
        if(timeout == 0) {
            std::vector<FS::EpollEvent> epollEvents;
            auto descriptor = currentProcess_->fds()[epfd];
            int ret = kernel_.fs().epollWaitImmediate(descriptor, &epollEvents);
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
                mmu_->writeToMmu<EpollEvent>(events, eventsForMemory);
            }
            if(kernel_.logSyscalls()) {
                print("Sys::epoll_wait(epfd={}, events={:#x}, maxevents={}, timeout={})", epfd, events.address(), maxevents, timeout, ret);
            }
            return ret;
        } else {
            // int ret = kernel_.fs().epoll_wait(FD{epfd}, events, maxevents, timeout);
            kernel_.scheduler().epoll_wait(currentThread_, epfd, events, (size_t)maxevents, timeout);
            if(kernel_.logSyscalls()) {
                print("Sys::epoll_wait(epfd={}, events={:#x}, maxevents={}, timeout={}) = pending", epfd, events.address(), maxevents, timeout);
            }
            return 0;
        }
    }

    int Sys::epoll_ctl(int epfd, int op, int fd, mem::Ptr event) {
        verify(!!event, "Null event in epoll_ctl not supported");
        EpollEvent ee = mmu_->readFromMmu<EpollEvent>(event);
        auto epDescriptor = currentProcess_->fds()[epfd];
        auto descriptor = currentProcess_->fds()[fd];
        int ret = kernel_.fs().epoll_ctl(epDescriptor, op, descriptor, BitFlags<EpollEventType>::fromIntegerType(ee.event), ee.data);
        if(kernel_.logSyscalls()) {
            print("Sys::epoll_ctl(epfd={}, op={}, fd={}, event=[event={:#x}, data={}]) = {}", epfd, op, fd, ee.event, ee.data, ret);
        }
        return ret;
    }

    int Sys::tgkill(int tgid, int tid, int sig) {
        if(kernel_.logSyscalls()) print("Sys::tgkill(tgid={}, tid={}, sig={})", tgid, tid, sig);
        kernel_.scheduler().kill(tgid, tid, sig);
        return 0;
    }

    int Sys::mbind(unsigned long start, unsigned long len, unsigned long mode, mem::Ptr64 nmask, unsigned long maxnode, unsigned flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::mbind(start={}, len={}, mode={}, nmask={:#x}, maxnode={}, flags={})", start, len, mode, nmask.address(), maxnode, flags);
        }
        warn("mbind not implemented");
        return -ENOTSUP;
    }

    long Sys::get_mempolicy(mem::Ptr mode, mem::Ptr nodemask, unsigned long maxnode, mem::Ptr addr, unsigned long flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::get_mempolicy(mode={:#x}, nodemask={:#x}, maxnode={}, addr={:#x}, flags={})", mode.address(), nodemask.address(), maxnode, addr.address(), flags);
        }
        warn("get_mempolicy not implemented");
        return -ENOTSUP;
    }

    int Sys::waitid(int idtype, id_t id, mem::Ptr infop, int options, mem::Ptr rusage) {
        if(kernel_.logSyscalls()) {
            print("Sys::waitid(idtype={}, id={}, infop={:#x}, options={}, rusage={:#x}) = {}",
                    idtype, id, infop.address(), options, rusage.address(), -ENOTSUP);
        }
        warn("waitid not implemented");
        return -ENOTSUP;
    }

    int Sys::inotify_init() {
        auto fd = currentProcess_->fds().inotify_init1(0);
        if(kernel_.logSyscalls()) {
            print("Sys::inotify_init() = {}", fd.fd);
        }
        return fd.fd;
    }

    int Sys::inotify_add_watch(int fd, mem::Ptr pathname, uint32_t mask) {
        if(kernel_.logSyscalls()) print("Sys::inotify_add_watch(fd={}, pathname={}, mask={}) = {}", fd, mmu_->readString(pathname), mask, -ENOTSUP);
        warn("inotify_add_watch not implemented");
        return -ENOTSUP;
    }

    ssize_t Sys::getxattr(mem::Ptr path, mem::Ptr name, mem::Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto errnoOrBuffer = Host::getxattr(spath, sname, size);
        if(kernel_.logSyscalls()) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::lgetxattr(mem::Ptr path, mem::Ptr name, mem::Ptr value, size_t size) {
        auto spath = mmu_->readString(path);
        auto sname = mmu_->readString(name);
        auto errnoOrBuffer = Host::lgetxattr(spath, sname, size);
        if(kernel_.logSyscalls()) {
            print("Sys::getxaddr(path={}, name={}, value={:#x}, size={}) = {}",
                                      spath, sname, value.address(), size, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) {
                                        return (ssize_t)buffer.size();
                                      }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(value, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    ssize_t Sys::listxattr(mem::Ptr path, mem::Ptr list, size_t size) {
        // auto spath = mmu_->readString(path);
        // auto slist = mmu_->readString(list);
        if(kernel_.logSyscalls()) {
            print("Sys::listxattr(path={:#x}, list={:#x}, size={}) = {}",
                                      path.address(), list.address(), size, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    time_t Sys::time(mem::Ptr tloc) {
        time_t t = (time_t)kernel_.scheduler().kernelTime().seconds;
        if(kernel_.logSyscalls()) print("Sys::time({:#x}) = {}", tloc.address(), t);
        if(tloc.address()) mmu_->copyToMmu(tloc, (const u8*)&t, sizeof(t));
        return t;
    }

    long Sys::futex(mem::Ptr32 uaddr, int futex_op, uint32_t val, mem::Ptr timeout, mem::Ptr32 uaddr2, uint32_t val3) {
        auto onExit = [&](long ret) -> long {
            if(!kernel_.logSyscalls()) return ret;
            std::string op;
            switch(futex_op & 0x7f) {
                case 0: op = "wait"; break;
                case 1: op = "wake"; break;
                case 5: op = "wake_op"; break;
                case 9: op = "wait_bitset"; break;
                default: op = fmt::format("unknown futex {}", futex_op); break;
            }
            print("Sys::futex(uaddr={:#x}, op={}, val={}, timeout={:#x}, uaddr2={:#x}, val3={}) = {}",
                              uaddr.address(), op, val, timeout.address(), uaddr2.address(), val3, ret);
            return ret;
        };
        int unmaskedOp = futex_op & 0x7f;
        if(unmaskedOp == 0) {
            // wait
            u32 loaded = mmu_->read32(uaddr);
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
            u32 nbWoken = kernel_.scheduler().wakeOp(currentThread_, uaddr, val, uaddr2, val2, val3);
            return onExit(nbWoken);
        }
        if(unmaskedOp == 7) {
            warn("futex_unlock_pi returns bogus ENOSYS value");
            return onExit(-ENOSYS);
        }
        if(unmaskedOp == 9 && val3 == std::numeric_limits<uint32_t>::max()) {
            // wait_bitset
            u32 loaded = mmu_->read32(uaddr);
            if(loaded != val) return -EAGAIN;
            // create timer 0
            Timer* timer = kernel_.timers().getOrTryCreate(0);
            verify(!!timer);
            timer->update(kernel_.scheduler().kernelTime());
            kernel_.scheduler().waitBitset(currentThread_, uaddr, val, timeout);
            return onExit(0);
        }
        verify(false, [&]() {
            fmt::print("futex with op={} is not supported", unmaskedOp);
        });
        return 1;
    }

    int Sys::sched_setaffinity(pid_t pid, size_t cpusetsize, mem::Ptr mask) {
        if(kernel_.logSyscalls()) {
            print("Sys::sched_setaffinity(pid={}, cpusetsize={}, mask={:#x}) = {}", pid, cpusetsize, mask.address(), -ENOTSUP);
        }
        warn("sched_setaffinity not implemented");
        return -ENOTSUP;
    }

    int Sys::sched_getaffinity(pid_t pid, size_t cpusetsize, mem::Ptr mask) {
        int ret = 0;
        if(pid == 0) {
            // pretend that only cpu 0 is available.
            std::vector<u8> buffer;
            buffer.resize(cpusetsize, 0x0);
            if(!buffer.empty()) {
                buffer[0] |= 0x1;
            }
            mmu_->copyToMmu(mask, buffer.data(), buffer.size());
            ret = 1;
        } else {
            // don't allow looking at other processes
            ret = -EPERM;
        }
        if(kernel_.logSyscalls()) print("Sys::sched_getaffinity({}, {}, {:#x}) = {}",
                pid, cpusetsize, mask.address(), ret);
        return ret;
    }

    ssize_t Sys::recvfrom(int sockfd, mem::Ptr buf, size_t len, int flags, mem::Ptr src_addr, mem::Ptr32 addrlen) {
        bool requireSrcAddress = !!src_addr && !!addrlen;
        auto descriptor = currentProcess_->fds()[sockfd];
        ErrnoOr<std::pair<Buffer, Buffer>> ret = kernel_.fs().recvfrom(descriptor, len, flags, requireSrcAddress);
        if(kernel_.logSyscalls()) {
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

    ssize_t Sys::sendmsg(int sockfd, mem::Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header = mmu_->readFromMmu<msghdr>(msg);

        FS::Message message;

        // read Message::msg_name
        if(!!header.msg_name && header.msg_namelen > 0) {
            Buffer msg_name_buffer(header.msg_namelen, 0x0);
            mmu_->copyFromMmu(msg_name_buffer.data(), mem::Ptr8{(u64)header.msg_name}, msg_name_buffer.size());
            message.msg_name = std::move(msg_name_buffer);
        }

        // read Message::msg_iov
        std::vector<iovec> msg_iovecs = mmu_->readFromMmu<iovec>(mem::Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            Buffer msg_iovec_buffer(msg_iovecs[i].iov_len, 0x0);
            mmu_->copyFromMmu(msg_iovec_buffer.data(), mem::Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iovec_buffer.size());
            message.msg_iov.push_back(std::move(msg_iovec_buffer));
        }

        // read Message::control
        if(!!header.msg_control && header.msg_controllen > 0) {
            Buffer msg_control_buffer(header.msg_controllen, 0x0);
             mmu_->copyFromMmu(msg_control_buffer.data(), mem::Ptr8{(u64)header.msg_control}, msg_control_buffer.size());
            message.msg_control = std::move(msg_control_buffer);
        }

        // read Message::msg_flags
        message.msg_flags = header.msg_flags;

        auto descriptor = currentProcess_->fds()[sockfd];
        ssize_t nbytes = kernel_.fs().sendmsg(descriptor, flags, message);
        if(kernel_.logSyscalls()) {
            print("Sys::sendmsg(sockfd={}, msg={:#x}, flags={}) = {}",
                        sockfd, msg.address(), flags, nbytes);
        }
        return nbytes;
    }

    ssize_t Sys::recvmsg(int sockfd, mem::Ptr msg, int flags) {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header = mmu_->readFromMmu<msghdr>(msg);

        FS::Message message;

        // read Message::msg_name
        if(!!header.msg_name && header.msg_namelen > 0) {
            Buffer msg_name_buffer(header.msg_namelen, 0x0);
            mmu_->copyFromMmu(msg_name_buffer.data(), mem::Ptr8{(u64)header.msg_name}, msg_name_buffer.size());
            message.msg_name = std::move(msg_name_buffer);
        }

        // read Message::msg_iov
        std::vector<iovec> msg_iovecs = mmu_->readFromMmu<iovec>(mem::Ptr8{(u64)header.msg_iov}, header.msg_iovlen);
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            Buffer msg_iovec_buffer(msg_iovecs[i].iov_len, 0x0);
            mmu_->copyFromMmu(msg_iovec_buffer.data(), mem::Ptr8{(u64)msg_iovecs[i].iov_base}, msg_iovec_buffer.size());
            message.msg_iov.push_back(Buffer(std::move(msg_iovec_buffer)));
        }

        // read Message::control
        if(!!header.msg_control && header.msg_controllen > 0) {
            Buffer msg_control_buffer(header.msg_controllen, 0x0);
            mmu_->copyFromMmu(msg_control_buffer.data(), mem::Ptr8{(u64)header.msg_control}, msg_control_buffer.size());
            message.msg_control = std::move(msg_control_buffer);
        }

        // read Message::msg_flags
        message.msg_flags = header.msg_flags;

        // do the syscall
        auto descriptor = currentProcess_->fds()[sockfd];
        ssize_t nbytes = kernel_.fs().recvmsg(descriptor, flags, &message);

        // write back to header
        header.msg_namelen = (socklen_t)message.msg_name.size();
        if(!!header.msg_name) {
            mmu_->copyToMmu(mem::Ptr8{(u64)header.msg_name}, message.msg_name.data(), message.msg_name.size());
        }
        header.msg_iovlen = message.msg_iov.size();
        verify(header.msg_iovlen == message.msg_iov.size(), "message iov changed length...");
        for(size_t i = 0; i < header.msg_iovlen; ++i) {
            mmu_->copyToMmu(mem::Ptr8{(u64)msg_iovecs[i].iov_base}, message.msg_iov[i].data(), message.msg_iov[i].size());
        }
        header.msg_controllen = message.msg_control.size();
        if(!!header.msg_control) {
            mmu_->copyToMmu(mem::Ptr8{(u64)header.msg_control}, message.msg_control.data(), message.msg_control.size());
        }
        header.msg_flags = message.msg_flags;

        mmu_->writeToMmu<msghdr>(msg, header);
        
        if(kernel_.logSyscalls()) {
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
            print("Sys::recvmsg(sockfd={}, msg=[{}], flags={:#x}) = {}",
                        sockfd, messageString, flags, nbytes);
        }
        return nbytes;
    }

    int Sys::shutdown(int sockfd, int how) {
        auto descriptor = currentProcess_->fds()[sockfd];
        int rc = kernel_.fs().shutdown(descriptor, how);
        if(kernel_.logSyscalls()) {
            print("Sys::shutdown(sockfd={}, how={}) = {}",
                        sockfd, how, rc);
        }
        return rc;
    }

    int Sys::bind(int sockfd, mem::Ptr addr, socklen_t addrlen) {
        Buffer saddr(addrlen, 0x0);
        mmu_->copyFromMmu(saddr.data(), addr, saddr.size());
        auto descriptor = currentProcess_->fds()[sockfd];
        int rc = kernel_.fs().bind(descriptor, saddr);
        if(kernel_.logSyscalls()) {
            print("Sys::bind(sockfd={}, addr={:#x}, addrlen={}) = {}",
                        sockfd, addr.address(), addrlen, rc);
        }
        return rc;
    }

    int Sys::listen(int sockfd, int backlog) {
        if(kernel_.logSyscalls()) {
            print("Sys::listen(sockfd={}, backlog={}) = {}", sockfd, backlog, -ENOTSUP);
        }
        warn("listen not implemented");
        return -ENOTSUP;
    }

    ssize_t Sys::getdents64(int fd, mem::Ptr dirp, size_t count) {
        auto descriptor = currentProcess_->fds()[fd];
        auto errnoOrBuffer = kernel_.fs().getdents64(descriptor, count);
        if(kernel_.logSyscalls()) {
            print("Sys::getdents64(fd={}, dirp={:#x}, count={}) = {}",
                        fd, dirp.address(), count, errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) { return (ssize_t)buffer.size(); }));
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(dirp, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    pid_t Sys::set_tid_address(mem::Ptr32 ptr) {
        if(kernel_.logSyscalls()) print("Sys::set_tid_address({:#x}) = {}", ptr.address(), currentThread_->description().tid);
        currentThread_->setClearChildTid(ptr);
        return currentThread_->description().tid;
    }

    int Sys::posix_fadvise(int fd, off_t offset, off_t len, int advice) {
        if(kernel_.logSyscalls()) {
            print("Sys::posix_fadvise(fd={}, offset={}, len={}, advise={}) = {}",
                                    fd, offset, len, advice, 0);
        }
        return 0;
    }

    int Sys::clock_gettime(clockid_t clockid, mem::Ptr tp) {
        // create the timer for future reference
        auto* timer = kernel_.timers().getOrTryCreate(clockid);
        if(!timer) return -EINVAL;
        // TODO: we should read from the timer, not the scheduler
        PreciseTime time = kernel_.scheduler().kernelTime();
        timer->update(time); // just in case
        Buffer buffer = Host::clock_gettime(time);
        mmu_->copyToMmu(tp, buffer.data(), buffer.size());
        if(kernel_.logSyscalls()) {
            print("Sys::clock_gettime({}, {:#x}) = {}",
                        clockid, tp.address(), 0);
        }
        return 0;
    }

    int Sys::clock_getres(clockid_t clockid, mem::Ptr res) {
        auto buffer = Host::clock_getres();
        if(kernel_.logSyscalls()) {
            print("Sys::clock_getres({}, {:#x}) = {}",
                        clockid, res.address(), 0);
        }
        mmu_->copyToMmu(res, buffer.data(), buffer.size());
        return 0;
    }

    int Sys::clock_nanosleep(clockid_t clockid, int flags, mem::Ptr request, mem::Ptr remain) {
        verify(flags == 0, "clock_nanosleep with nonzero flags not supported (relative only)");
        Timer* timer = kernel_.timers().getOrTryCreate(clockid);
        if(!timer) { return -EINVAL; }
        auto timediff = timer->readRelativeTimespec(*mmu_, request);
        if(!timediff) { return -EFAULT; }
        timer->update(kernel_.scheduler().kernelTime());
        kernel_.scheduler().sleep(currentThread_, timer, timer->now() + timediff.value());
        if(kernel_.logSyscalls()) {
            print("Sys::clock_nanosleep(clockid={}, flags={}, request={}s{}ns, remain={:#x}) = {}",
                        clockid, flags, timediff->seconds, timediff->nanoseconds, remain.address(), 0);
        }
        return 0;
    }

    int Sys::prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
        int ret = -ENOTSUP;
        if(Host::Prctl::isSetName(option)) {
            mem::Ptr8 ptr { arg2 };
            std::string threadName = mmu_->readString(ptr);
            if(threadName.size() >= 15) threadName.resize(15);
            currentThread_->setName(threadName);
            ret = 0;
        }
        if(Host::Prctl::isCapabilitySetRead(option)) {
            // No capabilities are allowed
            ret = 0;
        }
        if(kernel_.logSyscalls()) {
            print("Sys::prctl(option={}, arg2={}, arg3={}, arg4={}, arg5={}) = {}", option, arg2, arg3, arg4, arg5, ret);
        }
        if(ret == -ENOTSUP) {
            warn("prctl not implemented for this option");
        }
        return -ENOTSUP;
    }

    int Sys::arch_prctl(int code, mem::Ptr addr) {
        bool isSetFS = Host::ArchPrctl::isSetFS(code);
        if(kernel_.logSyscalls()) print("Sys::arch_prctl(code={}, addr={:#x}) = {}", code, addr.address(), isSetFS ? 0 : -EINVAL);
        if(!isSetFS) return -EINVAL;
        verify(!!currentThread_);
        currentThread_->setTlsBase(addr.address());
        return 0;
    }

    int Sys::gettid() {
        verify(!!currentThread_);
        int tid = currentThread_->description().tid;
        if(kernel_.logSyscalls()) print("Sys::gettid() = {}", tid);
        return tid;
    }

    int Sys::openat(int dirfd, mem::Ptr pathname, int flags, mode_t mode) {
        std::string path = mmu_->readString(pathname);
        BitFlags<AccessMode> accessMode = FS::toAccessMode(flags);
        BitFlags<CreationFlags> creationFlags = FS::toCreationFlags(flags);
        BitFlags<StatusFlags> statusFlags = FS::toStatusFlags(flags);
        Permissions permissions = FS::fromMode(mode);
        auto dirFd = currentProcess_->fds().dirfd(FD{dirfd}, currentProcess_->cwd());
        auto filepath = kernel_.fs().resolvePath(dirFd, path);
        FD fd = [&]() -> FD {
            if(!filepath) return FD{-ENOENT};
            return currentProcess_->fds().open(*filepath, accessMode, creationFlags, statusFlags, permissions);
        }();
        if(kernel_.logSyscalls()) {
            std::string flagsString = fmt::format("[{}{}{}{}{}{}{}]",
                accessMode.test(AccessMode::READ)  ? "Read " : "",
                accessMode.test(AccessMode::WRITE) ? "Write " : "",
                statusFlags.test(StatusFlags::APPEND) ? "Append " : "",
                creationFlags.test(CreationFlags::TRUNC) ? "Truncate " : "",
                creationFlags.test(CreationFlags::CREAT) ? "Create " : "",
                creationFlags.test(CreationFlags::CLOEXEC) ? "CloseOnExec " : "",
                creationFlags.test(CreationFlags::DIRECTORY) ? "Directory " : "");
            print("Sys::openat(dirfd={}, path={}, flags={}, mode={:o}) = {}", dirfd, path, flagsString, mode, fd.fd);
        }
        return fd.fd;
    }

    int Sys::fstatat64(int dirfd, mem::Ptr pathname, mem::Ptr statbuf, int flags) {
        std::string pathname_ = mmu_->readString(pathname);
        auto allowEmptyPath = Host::Fstatat::isEmptyPath(flags) ? FS::AllowEmptyPathname::YES : FS::AllowEmptyPathname::NO;
        auto dirFd = currentProcess_->fds().dirfd(FD{dirfd}, currentProcess_->cwd());
        auto path = kernel_.fs().resolvePath(dirFd, pathname_, allowEmptyPath);
        auto errnoOrBuffer = [&]() {
            if(!path) return ErrnoOrBuffer(-ENOENT);
            return kernel_.fs().fstatat64(*path, flags);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::fstatat64(dirfd={}, path={}, statbuf={:#x}, flags={}) = {}",
                        dirfd, pathname_, statbuf.address(), flags, errnoOrBuffer.errorOr(0));
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::unlinkat(int dirfd, mem::Ptr pathname, int flags) {
        if(kernel_.logSyscalls()) {
            std::string path = mmu_->readString(pathname);
            print("Sys::unlinkat(dirfd={}, path={}, flags={}) = {}",
                        dirfd, path, flags, -ENOTSUP);
        }
        warn("unlinkat not implemented");
        return -ENOTSUP;
    }

    int Sys::linkat(int olddirfd, mem::Ptr oldpath, int newdirfd, mem::Ptr newpath, int flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::linkat(olddirfd={}, oldpath={:#x}, newdirfd={:#x}, newpath={:#x}, flags={}) = {}",
                olddirfd, oldpath.address(), newdirfd, newpath.address(), flags, -ENOTSUP);
        }
        warn("linkat not implemented");
        return -ENOTSUP;
    }

    ssize_t Sys::readlinkat(int dirfd, mem::Ptr pathname, mem::Ptr buf, size_t bufsiz) {
        verify(dirfd == Host::cwdfd().fd, "dirfd is not cwd");
        std::string path = mmu_->readString(pathname);

        auto linkpath = kernel_.fs().resolvePath(currentProcess_->cwd(), path);
        auto errnoOrBuffer = [&]() {
            if(!linkpath) return ErrnoOrBuffer(-ENOENT);
            return kernel_.fs().readlink(*linkpath, bufsiz);
        }();

        // auto errnoOrBuffer = Host::readlink(path, bufsiz);
        if(kernel_.logSyscalls()) {
            print("Sys::readlinkat(dirfd={}, path={}, buf={:#x}, size={}) = {:#x}",
                        dirfd, path, buf.address(), bufsiz, errnoOrBuffer.errorOrWith<ssize_t>([](const auto& buffer) { return (ssize_t)buffer.size(); }));
            errnoOrBuffer.with([](const Buffer& buf) {
                std::string bufstr((const char*)buf.data());
                fmt::println("  buf={}", bufstr);
            });
        }
        return errnoOrBuffer.errorOrWith<ssize_t>([&](const auto& buffer) {
            mmu_->copyToMmu(buf, buffer.data(), buffer.size());
            return (ssize_t)buffer.size();
        });
    }

    int Sys::faccessat(int dirfd, mem::Ptr pathname, int mode) {
        std::string pathname_ = mmu_->readString(pathname);
        auto dirFd = currentProcess_->fds().dirfd(FD{dirfd}, currentProcess_->cwd());
        auto path = kernel_.fs().resolvePath(dirFd, pathname_);
        int ret = [&]() {
            if(!path) return -ENOENT;
            return kernel_.fs().access(*path, mode);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::faccessat(dirfd={}, path={}, mode={}) = {}", dirfd, pathname_, mode, ret);
        }
        return ret;
    }

    int Sys::pselect6(int nfds, mem::Ptr readfds, mem::Ptr writefds, mem::Ptr exceptfds, mem::Ptr timeout, mem::Ptr sigmask) {
        if(!sigmask) warn("non-null sigmask not supported in Sys::pselect6");
        static_assert(sizeof(FS::SelectData::readfds) == sizeof(fd_set));
        FS::SelectData selectData;
        selectData.fds.reserve(nfds);
        for(int fd = 0; fd < nfds; ++fd) {
            selectData.fds.push_back(currentProcess_->fds()[fd]);
        }
        if(!!readfds) mmu_->copyFromMmu((u8*)&selectData.readfds, readfds, sizeof(selectData.readfds));
        if(!!writefds) mmu_->copyFromMmu((u8*)&selectData.writefds, writefds, sizeof(selectData.writefds));
        if(!!exceptfds) mmu_->copyFromMmu((u8*)&selectData.exceptfds, exceptfds, sizeof(selectData.exceptfds));
        Timer* timer = kernel_.timers().getOrTryCreate(0);
        auto timeoutDuration = timer->readTimespec(*mmu_, timeout);
        if(!!timeoutDuration && timeoutDuration->seconds == 0 && timeoutDuration->nanoseconds == 0) {
            int ret = kernel_.fs().selectImmediate(&selectData);
            if(kernel_.logSyscalls()) {
                print("Sys::pselect6(nfds={}, readfds={:#x}, writefds={:#x}, exceptfds={:#x}, timeout={:#x}, sigmask={:#x}) = {}",
                            nfds, readfds.address(), writefds.address(), exceptfds.address(), timeout.address(), sigmask.address(), ret);
            }
            if(ret < 0) return ret;
            if(!!readfds) mmu_->copyToMmu(readfds, (const u8*)&selectData.readfds, sizeof(selectData.readfds));
            if(!!writefds) mmu_->copyToMmu(writefds, (const u8*)&selectData.writefds, sizeof(selectData.writefds));
            if(!!exceptfds) mmu_->copyToMmu(exceptfds, (const u8*)&selectData.exceptfds, sizeof(selectData.exceptfds));
            return ret;
        } else {
            kernel_.scheduler().pselect(currentThread_, nfds, readfds, writefds, exceptfds, timeout);
            return 0;
        }
    }

    int Sys::ppoll(mem::Ptr fds, int nfds, mem::Ptr tmo_p, mem::Ptr sigmask, size_t sigsetsize) {
        verify(!sigmask, "Sys::ppoll does not support non-null sigmask");
        assert(sizeof(FS::PollFd) == Host::pollRequiredBufferSize(1));
        std::vector<FS::PollFd> pollfds = mmu_->readFromMmu<FS::PollFd>(fds, (size_t)nfds);
        Timer* timer = kernel_.timers().getOrTryCreate(0);
        verify(!!timer);
        auto timeoutDuration = timer->readRelativeTimespec(*mmu_, tmo_p);
        int timeoutInMs = -1;
        if(!!timeoutDuration) {
            timeoutInMs = (int)(timeoutDuration->seconds*1'000 + timeoutDuration->nanoseconds / 1'000'000);
        }
        if(kernel_.logSyscalls()) {
            print("Sys::ppoll(fds={:#x}, nfds={}, timeout={:#x}, sigmask={:#x}, sigsetsize={}) = pending",
                        fds.address(), nfds, tmo_p.address(), sigmask.address(), sigsetsize);
        }
        kernel_.scheduler().poll(currentThread_, fds, (size_t)nfds, timeoutInMs);
        return 0;
    }

    long Sys::set_robust_list(mem::Ptr head, size_t len) {
        if(kernel_.logSyscalls()) print("Sys::set_robust_list({:#x}, {}) = 0", head.address(), len);
        currentThread_->setRobustList(head, len);
        return 0;
    }

    long Sys::get_robust_list(int pid, mem::Ptr64 head_ptr, mem::Ptr64 len_ptr) {
        if(kernel_.logSyscalls()) print("Sys::get_robust_list({}, {:#x}, {:#x}) = 0", pid, head_ptr.address(), len_ptr.address());
        (void)pid;
        (void)head_ptr;
        (void)len_ptr;
        verify(false, "implement {get,set}_robust_list");
        return 0;
    }

    int Sys::utimensat(int dirfd, mem::Ptr pathname, mem::Ptr times, int flags) {
        if(kernel_.logSyscalls()) {
            std::string path = !!pathname ? mmu_->readString(pathname) : "NULL";
            print("Sys::utimensat(dirfd={}, pathname={}, times={:#x}, flags={}) = -ENOTSUP",
                                                          dirfd, path, times.address(), flags);
        }
        warn("utimensat not implemented");
        return -ENOTSUP;
    }

    int Sys::fallocate(int fd, int mode, off_t offset, off_t len) {
        auto descriptor = currentProcess_->fds()[fd];
        int ret = kernel_.fs().fallocate(descriptor, mode, offset, len);
        if(kernel_.logSyscalls()) {
            print("Sys::fallocate(fd={}, mode={}, offset={:#x}, len={}) = {}",
                                                          fd, mode, offset, len, ret);
        }
        return ret;
    }

    int Sys::eventfd2(unsigned int initval, int flags) {
        FD fd = currentProcess_->fds().eventfd2(initval, flags);
        if(kernel_.logSyscalls()) {
            print("Sys::eventfd2(initval={}, flags={}) = {}", initval, flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::epoll_create1(int flags) {
        FD fd = currentProcess_->fds().epoll_create1(flags);
        if(kernel_.logSyscalls()) {
            print("Sys::epoll_create1(flags={}) = {}", flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::dup3(int oldfd, int newfd, int flags) {
        FD fd = currentProcess_->fds().dup3(FD{oldfd}, FD{newfd}, flags);
        if(kernel_.logSyscalls()) {
            print("Sys::dup3(oldfd={}, newfd={}, flags={}) = {}", oldfd, newfd, flags, fd.fd);
        }
        return fd.fd;
    }
    
    int Sys::pipe2(mem::Ptr32 pipefd, int flags) {
        auto errnoOrFds = currentProcess_->fds().pipe2(flags);
        int ret = errnoOrFds.errorOrWith<int>([&](std::pair<FD, FD> fds) {
            std::vector<u32> fdsbuf {{ (u32)fds.first.fd, (u32)fds.second.fd }};
            mem::Ptr ptr { pipefd.address() };
            mmu_->writeToMmu(ptr, fdsbuf);
            return 0;
        });
        if(kernel_.logSyscalls()) {
            print("Sys::pipe2(pipefd={:#x}, flags={}) = {}", pipefd.address(), flags, ret);
        }
        return ret;
    }

    int Sys::inotify_init1(int flags) {
        if(kernel_.logSyscalls()) {
            print("Sys::inotify_init1(flags={}) = {}", flags, -ENOTSUP);
        }
        return -ENOTSUP;
    }

    int Sys::prlimit64(pid_t pid, int resource, mem::Ptr new_limit, mem::Ptr old_limit) {
        if(kernel_.logSyscalls()) 
            print("Sys::prlimit64(pid={}, resource={}, new_limit={:#x}, old_limit={:#x})", pid, resource, new_limit.address(), old_limit.address());
        if(!old_limit.address()) {
            if(kernel_.logSyscalls()) print(" = 0");
            return 0;
        }
        auto errnoOrBuffer = Host::getrlimit(pid, resource);
        if(kernel_.logSyscalls()) print(" = {}", errnoOrBuffer.errorOr(0));
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(old_limit, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::sched_setattr(pid_t pid, mem::Ptr attr, unsigned int flags) {
        if(kernel_.logSyscalls()) {
            Host::SchedAttr attributes = mmu_->readFromMmu<Host::SchedAttr>(attr);
            std::string attributeString = fmt::format("policy={} flags={} nice={} priority={}", attributes.schedPolicy, attributes.schedFlags, attributes.schedNice, attributes.schedPriority);
            print("Sys::sched_setattr(pid={}, attr={:#x} ({}), flags={:#x}) = 0", pid, attr.address(), attributeString, flags);
        }
        return 0;
    }

    int Sys::sched_getattr(pid_t pid, mem::Ptr attr, unsigned int size, unsigned int flags) {
        Host::SchedAttr attributes = Host::getSchedulerAttributes();
        if(size < sizeof(attributes)) {
            if(kernel_.logSyscalls()) 
                print("Sys::sched_getattr(pid={}, attr={:#x}, size={:#x}, flags={:#x}) = {}", pid, attr.address(), size, flags, -EINVAL);
            return -EINVAL;
        }
        mmu_->writeToMmu<Host::SchedAttr>(attr, attributes);
        if(kernel_.logSyscalls()) 
            print("Sys::sched_getattr(pid={}, attr={:#x}, size={:#x}, flags={:#x}) = 0", pid, attr.address(), size, flags);
        return 0;
    }

    ssize_t Sys::getrandom(mem::Ptr buf, size_t len, int flags) {
        if(kernel_.logSyscalls()) 
            print("Sys::getrandom(buf={:#x}, len={}, flags={})", buf.address(), len, flags);
        std::vector<u8> buffer(len);
        std::iota(buffer.begin(), buffer.end(), 0);
        mmu_->copyToMmu(buf, buffer.data(), buffer.size());
        return (ssize_t)len;
    }

    int Sys::memfd_create(mem::Ptr name, unsigned int flags) {
        auto filename = mmu_->readString(name);
        FD fd = currentProcess_->fds().memfd_create(filename, flags);
        if(kernel_.logSyscalls()) {
            std::string pathname = mmu_->readString(name);
            print("Sys::memfd_create(name={}, flags={:#x}) = {}", pathname, flags, fd.fd);
        }
        return fd.fd;
    }

    int Sys::statx(int dirfd, mem::Ptr pathname, int flags, unsigned int mask, mem::Ptr statxbuf) {
        std::string pathname_ = mmu_->readString(pathname);
        auto allowEmptyPath = Host::Fstatat::isEmptyPath(flags) ? FS::AllowEmptyPathname::YES : FS::AllowEmptyPathname::NO;
        auto dirFd = currentProcess_->fds().dirfd(FD{dirfd}, currentProcess_->cwd());
        auto path = kernel_.fs().resolvePath(dirFd, pathname_, allowEmptyPath);
        auto errnoOrBuffer = [&]() {
            if(!path) return ErrnoOrBuffer(-ENOENT);
            return kernel_.fs().statx(*path, flags, mask);
        }();
        if(kernel_.logSyscalls()) {
            print("Sys::statx(dirfd={}, path={}, flags={}, mask={}, statxbuf={:#x}) = {}",
                        dirfd, pathname_, flags, mask, statxbuf.address(), errnoOrBuffer.errorOr(0));
        }
        if(errnoOrBuffer.errorOr(0) == -ENOTSUP) {
            warn("statx not supported on {}", pathname_);
        }
        return errnoOrBuffer.errorOrWith<int>([&](const auto& buffer) {
            mmu_->copyToMmu(statxbuf, buffer.data(), buffer.size());
            return 0;
        });
    }

    int Sys::rseq(mem::Ptr rseq, uint32_t rseq_len, int flags, uint32_t sig) {
        warn("rseq not implemented");
        if(kernel_.logSyscalls()) {
            print("Sys::rseq(rseq={:#x}, rseq_len={}, flags={}, sig={}) = {}",
                        rseq.address(), rseq_len, flags, sig, 0);
        }
        return 0;
    }

    int Sys::clone3(mem::Ptr uargs, size_t size) {
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
        verify(args.size() >= 8);
        u64 flags { args[0] };
        mem::Ptr32 child_tid { args[2] };
        mem::Ptr32 parent_tid { args[3] };
        u64 stackAddress = args[5] + args[6];
        u64 tls = args[7];

        Host::CloneFlags cloneFlags = Host::fromCloneFlags(flags);
        Thread* newThread { nullptr };
        if(cloneFlags.cloneThread) {
            verify(cloneFlags.cloneSignalHandlers, "cloneThread implies cloneSignalHandlers");
            verify(cloneFlags.cloneVm, "cloneThread implies cloneVm");
            verify(!!currentProcess_);
            verify(!!currentThread_);
            verify(checkCloneFlags(cloneFlags));

            newThread = currentProcess_->addThread(kernel_.processTable());
        } else {
            verify(!!currentProcess_);
            verify(!!currentThread_);
            verify(!cloneFlags.cloneFiles, "cloneFiles without cloneThread not supported");
            BitFlags<Process::CloneFlags> cflags;
            if(cloneFlags.cloneVm) cflags.add(Process::CloneFlags::VM);
            Process* newProcess = [&]() -> Process* {
                auto process = currentProcess_->clone(kernel_.processTable(), cflags);
                verify(!!process, "Unable to create new process");
                return kernel_.processTable().addProcess(std::move(process));
            }();
            bool flagsOk = checkCloneFlagsFork(cloneFlags);
            if(!flagsOk) {
                if(kernel_.logSyscalls()) {
                    print("Sys::clone(flags={}, stack={:#x}, parent_tid={:#x}, child_tid={:#x}, tls={}) = {}",
                                flags, stackAddress, parent_tid.address(), child_tid.address(), tls, -ENOTSUP);
                }
                return -ENOTSUP;
            }
            newThread = newProcess->addThread(kernel_.processTable());
            if(cloneFlags.cloneVfork) {
                kernel_.scheduler().suspendUntilVmReleased(currentThread_);
            }
        }

        verify(!!newThread);
        mem::Mmu childMmu(newThread->process()->addressSpace());
        newThread->cloneState(*currentThread_);
        newThread->setSyscallOutput(0);
        newThread->setStackPtr(stackAddress);
        mmu_->setRegionName(stackAddress-0x8, fmt::format("Stack of thread {}", newThread->description().tid));
        if(cloneFlags.setTls) {
            newThread->setTlsBase(tls);
        }
        if(cloneFlags.childClearTid) {
            newThread->setClearChildTid(child_tid);
        }
        if(!!child_tid && cloneFlags.childSetTid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            childMmu.write32(child_tid, (u32)newThread->description().tid);
        }
        long ret = newThread->description().tid;
        if(!!child_tid) {
            static_assert(sizeof(pid_t) == sizeof(u32));
            mmu_->write32(child_tid, (u32)ret);
        }
        if(kernel_.logSyscalls()) {
            print("Sys::clone3(uargs={:#x}, size={}) = {}",
                        uargs.address(), size, ret);
        }
        kernel_.scheduler().addThread(newThread);
        return (int)ret;
    }

}