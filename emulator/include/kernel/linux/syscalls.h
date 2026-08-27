#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "utils.h"
#include "x64/types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <tuple>
#include <unistd.h>
#include <sys/types.h>

namespace mem {
    class Mmu;
}

namespace kernel {
    class Process;
    class Scheduler;
    class Thread;
}

namespace kernel::gnulinux {
    class Host;
    class Kernel;

    class Sys {
    public:
        explicit Sys(Kernel& kernel);

        void syscall(Process* process, Thread* thread);

    private:
        struct RegisterDump {
            std::array<u64, 6> args;
        };

        template <typename ReturnType, typename... Args>
        struct function_traits_defs {
            static constexpr size_t arity = sizeof...(Args);

            using result_type = ReturnType;

            template <size_t i>
            struct arg {
                using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
            };
        };

        template <typename T>
        struct function_traits_impl;

        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits_impl<ReturnType(ClassType::*)(Args...)> : function_traits_defs<ReturnType, Args...> {};

        template<typename T>
        u64 get_value_or_address(T&& t) {
            if constexpr(std::is_same_v<T, mem::Ptr>) {
                return t.address();
            } else {
                static_assert(std::is_integral_v<T>);
                return (u64)t;
            }
        }

        template<typename Func>
        u64 invoke_syscall_0(Func&& func, const RegisterDump&) {
            return get_value_or_address((this->*func)());
        }

        template<typename Func>
        u64 invoke_syscall_1(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0]));
        }

        template<typename Func>
        u64 invoke_syscall_2(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            using arg1_t = typename function_traits_impl<Func>::template arg<1>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0], (arg1_t)regs.args[1]));
        }

        template<typename Func>
        u64 invoke_syscall_3(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            using arg1_t = typename function_traits_impl<Func>::template arg<1>::type;
            using arg2_t = typename function_traits_impl<Func>::template arg<2>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0], (arg1_t)regs.args[1], (arg2_t)regs.args[2]));
        }

        template<typename Func>
        u64 invoke_syscall_4(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            using arg1_t = typename function_traits_impl<Func>::template arg<1>::type;
            using arg2_t = typename function_traits_impl<Func>::template arg<2>::type;
            using arg3_t = typename function_traits_impl<Func>::template arg<3>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0], (arg1_t)regs.args[1], (arg2_t)regs.args[2], (arg3_t)regs.args[3]));
        }

        template<typename Func>
        u64 invoke_syscall_5(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            using arg1_t = typename function_traits_impl<Func>::template arg<1>::type;
            using arg2_t = typename function_traits_impl<Func>::template arg<2>::type;
            using arg3_t = typename function_traits_impl<Func>::template arg<3>::type;
            using arg4_t = typename function_traits_impl<Func>::template arg<4>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0], (arg1_t)regs.args[1], (arg2_t)regs.args[2], (arg3_t)regs.args[3], (arg4_t)regs.args[4]));
        }

        template<typename Func>
        u64 invoke_syscall_6(Func&& func, const RegisterDump& regs) {
            using arg0_t = typename function_traits_impl<Func>::template arg<0>::type;
            using arg1_t = typename function_traits_impl<Func>::template arg<1>::type;
            using arg2_t = typename function_traits_impl<Func>::template arg<2>::type;
            using arg3_t = typename function_traits_impl<Func>::template arg<3>::type;
            using arg4_t = typename function_traits_impl<Func>::template arg<4>::type;
            using arg5_t = typename function_traits_impl<Func>::template arg<5>::type;
            return get_value_or_address((this->*func)((arg0_t)regs.args[0], (arg1_t)regs.args[1], (arg2_t)regs.args[2], (arg3_t)regs.args[3], (arg4_t)regs.args[4], (arg5_t)regs.args[5]));
        }


        // 0x0
        ssize_t read(int fd, mem::Ptr buf, size_t count);
        // 0x1
        ssize_t write(int fd, mem::Ptr buf, size_t count);
        // 0x2
        int open(mem::Ptr pathname, int flags, mode_t mode);
        // 0x3
        int close(int fd);
        // 0x3
        int stat(mem::Ptr pathname, mem::Ptr statbuf);
        // 0x5
        int fstat(int fd, mem::Ptr statbuf);
        // 0x6
        int lstat(mem::Ptr pathname, mem::Ptr statbuf);
        // 0x7
        int poll(mem::Ptr fds, size_t nfds, int timeout);
        // 0x8
        off_t lseek(int fd, off_t offset, int whence);
        // 0x9
        mem::Ptr mmap(mem::Ptr addr, size_t length, int prot, int flags, int fd, off_t offset);
        // 0xa
        int mprotect(mem::Ptr addr, size_t length, int prot);
        // 0xb
        int munmap(mem::Ptr addr, size_t length);
        // 0xc
        mem::Ptr brk(mem::Ptr addr);
        // 0xd
        int rt_sigaction(int sig, mem::Ptr act, mem::Ptr oact, size_t sigsetsize);
        // 0xe
        int rt_sigprocmask(int how, mem::Ptr nset, mem::Ptr oset, size_t sigsetsize);
        // 0x10
        int ioctl(int fd, unsigned long request, mem::Ptr argp);
        // 0x11
        ssize_t pread64(int fd, mem::Ptr buf, size_t count, off_t offset);
        // 0x12
        ssize_t pwrite64(int fd, mem::Ptr buf, size_t count, off_t offset);
        // 0x13
        ssize_t readv(int fd, mem::Ptr iov, int iovcnt);
        // 0x14
        ssize_t writev(int fd, mem::Ptr iov, int iovcnt);
        // 0x15
        int access(mem::Ptr pathname, int mode);
        // 0x16
        int pipe(mem::Ptr32 pipefd);
        // 0x17
        int select(int nfds, mem::Ptr readfds, mem::Ptr writefds, mem::Ptr exceptfds, mem::Ptr timeout);
        // 0x18
        int sched_yield();
        // 0x19
        mem::Ptr mremap(mem::Ptr old_address, size_t old_size, size_t new_size, int flags, mem::Ptr new_address);
        // 0x1a
        int msync(mem::Ptr addr, size_t length, int flags);
        // 0x1b
        int mincore(mem::Ptr addr, size_t length, mem::Ptr8 vec);
        // 0x1c
        int madvise(mem::Ptr addr, size_t length, int advice);
        // 0x1d
        int shmget(key_t key, size_t size, int shmflg);
        // 0x1e
        mem::Ptr shmat(int shmid, mem::Ptr shmaddr, int shmflg);
        // 0x1f
        int shmctl(int shmid, int cmd, mem::Ptr buf);
        // 0x20
        int dup(int oldfd);
        // 0x21
        int dup2(int oldfd, int newfd);
        // 0x26
        int setitimer(int which, const mem::Ptr new_value, mem::Ptr old_value);
        // 0x27
        int getpid();
        // 0x29
        int socket(int domain, int type, int protocol);
        // 0x2a
        int connect(int sockfd, mem::Ptr addr, size_t addrlen);
        // 0x2c
        ssize_t sendto(int sockfd, mem::Ptr buf, size_t len, int flags, mem::Ptr dest_addr, socklen_t addrlen);
        // 0x2d
        ssize_t recvfrom(int sockfd, mem::Ptr buf, size_t len, int flags, mem::Ptr src_addr, mem::Ptr32 addrlen);
        // 0x2e
        ssize_t sendmsg(int sockfd, mem::Ptr msg, int flags);
        // 0x2f
        ssize_t recvmsg(int sockfd, mem::Ptr msg, int flags);
        // 0x30
        int shutdown(int sockfd, int how);
        // 0x31
        int bind(int sockfd, mem::Ptr addr, socklen_t addrlen);
        // 0x32
        int listen(int sockfd, int backlog);
        // 0x33
        int getsockname(int sockfd, mem::Ptr addr, mem::Ptr32 addrlen);
        // 0x34
        int getpeername(int sockfd, mem::Ptr addr, mem::Ptr32 addrlen);
        // 0x35
        int socketpair(int domain, int type, int protocol, mem::Ptr32 sv);
        // 0x36
        int setsockopt(int sockfd, int level, int optname, mem::Ptr optval, socklen_t optlen);
        // 0x37
        int getsockopt(int sockfd, int level, int optname, mem::Ptr optval, mem::Ptr32 optlen);
        // 0x38
        long clone(unsigned long flags, mem::Ptr stack, mem::Ptr32 parent_tid, mem::Ptr32 child_tid, unsigned long tls);
        // 0x39
        int fork();
        // 0x3a
        int vfork();
        // 0x3b
        int execve(mem::Ptr pathname, mem::Ptr64 argv, mem::Ptr64 envp);
        // 0x3c
        int exit(int status);
        // 0x3d
        int wait4(pid_t pid, mem::Ptr32 wstatus, int options, mem::Ptr rusage);
        // 0x3e
        int kill(pid_t pid, int sig);
        // 0x3f
        int uname(mem::Ptr buf);
        // 0x43
        int shmdt(mem::Ptr shmaddr);
        // 0x48
        int fcntl(int fd, int cmd, int arg);
        // 0x49
        int flock(int fd, int operation);
        // 0x4a
        int fsync(int fd);
        // 0x4b
        int fdatasync(int fd);
        // 0x4c
        int truncate(mem::Ptr8 path, off_t length);
        // 0x4d
        int ftruncate(int fd, off_t length);
        // 0x4f
        int getcwd(mem::Ptr buf, size_t size);
        // 0x50
        int chdir(mem::Ptr path);
        // 0x52
        int rename(mem::Ptr oldpath, mem::Ptr newpath);
        // 0x53
        int mkdir(mem::Ptr pathname, mode_t mode);
        // 0x57
        int unlink(mem::Ptr pathname);
        // 0x59
        ssize_t readlink(mem::Ptr pathname, mem::Ptr buf, size_t bufsiz);
        // 0x5a
        int chmod(mem::Ptr pathname, mode_t mode);
        // 0x5b
        int fchmod(int fd, mode_t mode);
        // 0x5c
        int chown(mem::Ptr pathname, uid_t owner, gid_t group);
        // 0x5d
        int fchown(int fd, uid_t owner, gid_t group);
        // 0x5f
        int umask(int mask);
        // 0x60
        int gettimeofday(mem::Ptr tv, mem::Ptr tz);
        // 0x62
        int getrusage(int who, mem::Ptr usage);
        // 0x63
        int sysinfo(mem::Ptr info);
        // 0x64
        clock_t times(mem::Ptr buf);
        // 0x66
        int getuid();
        // 0x68
        int getgid();
        // 0x6b
        int geteuid();
        // 0x6c
        int getegid();
        // 0x6d
        int setpgid(pid_t pid, pid_t pgid);
        // 0x6e
        int getppid();
        // 0x6f
        int getpgrp();
        // 0x73
        int getgroups(int size, mem::Ptr list);
        // 0x76
        int getresuid(mem::Ptr32 ruid, mem::Ptr32 euid, mem::Ptr32 suid);
        // 0x78
        int getresgid(mem::Ptr32 rgid, mem::Ptr32 egid, mem::Ptr32 sgid);
        // 0x79
        pid_t getpgid(pid_t pid);
        // 0x80
        int rt_sigtimedwait(mem::Ptr set, mem::Ptr info, mem::Ptr timeout);
        // 0x83
        int sigaltstack(mem::Ptr ss, mem::Ptr old_ss);
        // 0x84
        int utime(mem::Ptr filename, mem::Ptr times);
        // 0x89
        int statfs(mem::Ptr path, mem::Ptr buf);
        // 0x8a
        int fstatfs(int fd, mem::Ptr buf);
        // 0x8c
        int getpriority(int which, id_t who);
        // 0x8d
        int setpriority(int which, id_t who, int prio);
        // 0x8f
        int sched_getparam(pid_t pid, mem::Ptr param);
        // 0x90
        int sched_setscheduler(pid_t pid, int policy, mem::Ptr param);
        // 0x91
        int sched_getscheduler(pid_t pid);
        // 0x92
        int sched_get_priority_max(int policy);
        // 0x93
        int sched_get_priority_min(int policy);
        // 0x95
        int mlock(mem::Ptr addr, size_t len);
        // 0x96
        int munlock(mem::Ptr addr, size_t len);
        // 0x97
        int mlockall(int flags);
        // 0x98
        int munlockall(int flags);
        // 0x9e
        int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
        // 0x9e
        int arch_prctl(int code, mem::Ptr addr);
        // 0xba
        int gettid();
        // 0xbf
        ssize_t getxattr(mem::Ptr path, mem::Ptr name, mem::Ptr value, size_t size);
        // 0xc0
        ssize_t lgetxattr(mem::Ptr path, mem::Ptr name, mem::Ptr value, size_t size);
        // 0xc2
        ssize_t listxattr(mem::Ptr path, mem::Ptr list, size_t size);
        // 0xc9
        time_t time(mem::Ptr tloc);
        // 0xca
        long futex(mem::Ptr32 uaddr, int futex_op, uint32_t val, mem::Ptr timeout, mem::Ptr32 uaddr2, uint32_t val3);
        // 0xcb
        int sched_setaffinity(pid_t pid, size_t cpusetsize, mem::Ptr mask);
        // 0xcc
        int sched_getaffinity(pid_t pid, size_t cpusetsize, mem::Ptr mask);
        // 0xd9
        ssize_t getdents64(int fd, mem::Ptr dirp, size_t count);
        // 0xda
        pid_t set_tid_address(mem::Ptr32 tidptr);
        // 0xdd
        int posix_fadvise(int fd, off_t offset, off_t len, int advice);
        // 0xe4
        int clock_gettime(clockid_t clockid, mem::Ptr tp);
        // 0xe5
        int clock_getres(clockid_t clockid, mem::Ptr res);
        // 0xe6
        int clock_nanosleep(clockid_t clockid, int flags, mem::Ptr request, mem::Ptr remain);
        // 0xe7
        u64 exit_group(int status);
        // 0xe8
        int epoll_wait(int epfd, mem::Ptr events, int maxevents, int timeout);
        // 0xe9
        int epoll_ctl(int epfd, int op, int fd, mem::Ptr event);
        // 0xea
        int tgkill(int tgid, int tid, int sig);
        // 0xed
        int mbind(unsigned long start, unsigned long len, unsigned long mode, mem::Ptr64 nmask, unsigned long maxnode, unsigned flags);
        // 0xf7
        int waitid(int idtype, id_t id, mem::Ptr infop, int options, mem::Ptr rusage);
        // 0xfd
        int inotify_init();
        // 0xfe
        int inotify_add_watch(int fd, mem::Ptr pathname, uint32_t mask);
        // 0x101
        int openat(int dirfd, mem::Ptr pathname, int flags, mode_t mode);
        // 0x106
        int fstatat64(int dirfd, mem::Ptr pathname, mem::Ptr statbuf, int flags);
        // 0x107
        int unlinkat(int dirfd, mem::Ptr pathname, int flags);
        // 0x109
        int linkat(int olddirfd, mem::Ptr oldpath, int newdirfd, mem::Ptr newpath, int flags);
        // 0x10b
        ssize_t readlinkat(int dirfd, mem::Ptr pathname, mem::Ptr buf, size_t bufsiz);
        // 0x10d
        int faccessat(int dirfd, mem::Ptr pathname, int mode);
        // 0x10e
        int pselect6(int nfds, mem::Ptr readfds, mem::Ptr writefds, mem::Ptr exceptfds, mem::Ptr timeout, mem::Ptr sigmask);
        // 0x10f
        int ppoll(mem::Ptr fds, int nfds, mem::Ptr tmo_p, mem::Ptr sigmask, size_t sigsetsize);
        // 0x111
        long set_robust_list(mem::Ptr head, size_t len);
        // 0x112
        long get_robust_list(int pid, mem::Ptr64 head_ptr, mem::Ptr64 len_ptr);
        // 0x118
        int utimensat(int dirfd, mem::Ptr pathname, mem::Ptr times, int flags);
        // 0x11d
        int fallocate(int fd, int mode, off_t offset, off_t len);
        // 0x122
        int eventfd2(unsigned int initval, int flags);
        // 0x123
        int epoll_create1(int flags);
        // 0x124
        int dup3(int oldfd, int newfd, int flags);
        // 0x125
        int pipe2(mem::Ptr32 pipefd, int flags);
        // 0x126
        int inotify_init1(int flags);
        // 0x12e
        int prlimit64(pid_t pid, int resource, mem::Ptr new_limit, mem::Ptr old_limit);
        // 0x13a
        int sched_setattr(pid_t pid, mem::Ptr attr, unsigned int flags);
        // 0x13b
        int sched_getattr(pid_t pid, mem::Ptr attr, unsigned int size, unsigned int flags);
        // 0x13e
        ssize_t getrandom(mem::Ptr buf, size_t len, int flags);
        // 0x13f
        int memfd_create(mem::Ptr name, unsigned int flags);
        // 0x14c
        int statx(int dirfd, mem::Ptr pathname, int flags, unsigned int mask, mem::Ptr statxbuf);
        // 0x1b3
        int clone3(mem::Ptr uargs, size_t size);

    private:

        template<typename... Args>
        void print(const char* format, Args... args) const;

        template<typename... Args>
        void warn(const char* format, Args... args) const;

        Kernel& kernel_;
        std::mutex mutex_;
        Process* currentProcess_ { nullptr };
        Thread* currentThread_ { nullptr };
        mem::Mmu* mmu_ { nullptr };
    };

}

#endif