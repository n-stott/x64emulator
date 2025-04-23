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

namespace x64 {
    class Mmu;
}

namespace kernel {
    class Host;
    class Kernel;
    class Scheduler;
    class Thread;

    class Sys {
    public:
        Sys(Kernel& kernel, x64::Mmu& mmu);

        void syscall(Thread* thread);

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
            if constexpr(std::is_same_v<T, x64::Ptr>) {
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
        ssize_t read(int fd, x64::Ptr buf, size_t count);
        // 0x1
        ssize_t write(int fd, x64::Ptr buf, size_t count);
        // 0x3
        int close(int fd);
        // 0x3
        int stat(x64::Ptr pathname, x64::Ptr statbuf);
        // 0x5
        int fstat(int fd, x64::Ptr statbuf);
        // 0x6
        int lstat(x64::Ptr pathname, x64::Ptr statbuf);
        // 0x7
        int poll(x64::Ptr fds, size_t nfds, int timeout);
        // 0x8
        off_t lseek(int fd, off_t offset, int whence);
        // 0x9
        x64::Ptr mmap(x64::Ptr addr, size_t length, int prot, int flags, int fd, off_t offset);
        // 0xa
        int mprotect(x64::Ptr addr, size_t length, int prot);
        // 0xb
        int munmap(x64::Ptr addr, size_t length);
        // 0xc
        x64::Ptr brk(x64::Ptr addr);
        // 0xd
        int rt_sigaction(int sig, x64::Ptr act, x64::Ptr oact, size_t sigsetsize);
        // 0xe
        int rt_sigprocmask(int how, x64::Ptr nset, x64::Ptr oset, size_t sigsetsize);
        // 0x10
        int ioctl(int fd, unsigned long request, x64::Ptr argp);
        // 0x11
        ssize_t pread64(int fd, x64::Ptr buf, size_t count, off_t offset);
        // 0x12
        ssize_t pwrite64(int fd, x64::Ptr buf, size_t count, off_t offset);
        // 0x13
        ssize_t readv(int fd, x64::Ptr iov, int iovcnt);
        // 0x14
        ssize_t writev(int fd, x64::Ptr iov, int iovcnt);
        // 0x15
        int access(x64::Ptr pathname, int mode);
        // 0x16
        int pipe(x64::Ptr32 pipefd);
        // 0x17
        int select(int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout);
        // 0x18
        int sched_yield();
        // 0x19
        x64::Ptr mremap(x64::Ptr old_address, size_t old_size, size_t new_size, int flags, x64::Ptr new_address);
        // 0x1b
        int mincore(x64::Ptr addr, size_t length, x64::Ptr8 vec);
        // 0x1c
        int madvise(x64::Ptr addr, size_t length, int advice);
        // 0x1d
        int shmget(key_t key, size_t size, int shmflg);
        // 0x1e
        x64::Ptr shmat(int shmid, x64::Ptr shmaddr, int shmflg);
        // 0x1f
        int shmctl(int shmid, int cmd, x64::Ptr buf);
        // 0x20
        int dup(int oldfd);
        // 0x21
        int dup2(int oldfd, int newfd);
        // 0x26
        int setitimer(int which, const x64::Ptr new_value, x64::Ptr old_value);
        // 0x27
        int getpid();
        // 0x29
        int socket(int domain, int type, int protocol);
        // 0x2a
        int connect(int sockfd, x64::Ptr addr, size_t addrlen);
        // 0x2c
        ssize_t sendto(int sockfd, x64::Ptr buf, size_t len, int flags, x64::Ptr dest_addr, socklen_t addrlen);
        // 0x2d
        ssize_t recvfrom(int sockfd, x64::Ptr buf, size_t len, int flags, x64::Ptr src_addr, x64::Ptr32 addrlen);
        // 0x2e
        ssize_t sendmsg(int sockfd, x64::Ptr msg, int flags);
        // 0x2f
        ssize_t recvmsg(int sockfd, x64::Ptr msg, int flags);
        // 0x30
        int shutdown(int sockfd, int how);
        // 0x31
        int bind(int sockfd, x64::Ptr addr, socklen_t addrlen);
        // 0x32
        int listen(int sockfd, int backlog);
        // 0x33
        int getsockname(int sockfd, x64::Ptr addr, x64::Ptr32 addrlen);
        // 0x34
        int getpeername(int sockfd, x64::Ptr addr, x64::Ptr32 addrlen);
        // 0x35
        int socketpair(int domain, int type, int protocol, x64::Ptr32 sv);
        // 0x36
        int setsockopt(int sockfd, int level, int optname, x64::Ptr optval, socklen_t optlen);
        // 0x37
        int getsockopt(int sockfd, int level, int optname, x64::Ptr optval, x64::Ptr32 optlen);
        // 0x38
        long clone(unsigned long flags, x64::Ptr stack, x64::Ptr32 parent_tid, x64::Ptr32 child_tid, unsigned long tls);
        // 0x3c
        int exit(int status);
        // 0x3e
        int kill(pid_t pid, int sig);
        // 0x3f
        int uname(x64::Ptr buf);
        // 0x43
        int shmdt(x64::Ptr shmaddr);
        // 0x48
        int fcntl(int fd, int cmd, int arg);
        // 0x49
        int flock(int fd, int operation);
        // 0x4a
        int fsync(int fd);
        // 0x4b
        int fdatasync(int fd);
        // 0x4c
        int truncate(x64::Ptr8 path, off_t length);
        // 0x4d
        int ftruncate(int fd, off_t length);
        // 0x4f
        int getcwd(x64::Ptr buf, size_t size);
        // 0x50
        int chdir(x64::Ptr path);
        // 0x52
        int rename(x64::Ptr oldpath, x64::Ptr newpath);
        // 0x53
        int mkdir(x64::Ptr pathname, mode_t mode);
        // 0x57
        int unlink(x64::Ptr pathname);
        // 0x59
        ssize_t readlink(x64::Ptr pathname, x64::Ptr buf, size_t bufsiz);
        // 0x5a
        int chmod(x64::Ptr pathname, mode_t mode);
        // 0x5b
        int fchmod(int fd, mode_t mode);
        // 0x5c
        int chown(x64::Ptr pathname, uid_t owner, gid_t group);
        // 0x5f
        int umask(int mask);
        // 0x60
        int gettimeofday(x64::Ptr tv, x64::Ptr tz);
        // 0x62
        int getrusage(int who, x64::Ptr usage);
        // 0x63
        int sysinfo(x64::Ptr info);
        // 0x64
        clock_t times(x64::Ptr buf);
        // 0x66
        int getuid();
        // 0x68
        int getgid();
        // 0x6b
        int geteuid();
        // 0x6c
        int getegid();
        // 0x6e
        int getppid();
        // 0x6f
        int getpgrp();
        // 0x73
        int getgroups(int size, x64::Ptr list);
        // 0x76
        int getresuid(x64::Ptr32 ruid, x64::Ptr32 euid, x64::Ptr32 suid);
        // 0x78
        int getresgid(x64::Ptr32 rgid, x64::Ptr32 egid, x64::Ptr32 sgid);
        // 0x83
        int sigaltstack(x64::Ptr ss, x64::Ptr old_ss);
        // 0x84
        int utime(x64::Ptr filename, x64::Ptr times);
        // 0x89
        int statfs(x64::Ptr path, x64::Ptr buf);
        // 0x8a
        int fstatfs(int fd, x64::Ptr buf);
        // 0x8d
        int setpriority(int which, id_t who, int prio);
        // 0x8f
        int sched_getparam(pid_t pid, x64::Ptr param);
        // 0x90
        int sched_setscheduler(pid_t pid, int policy, x64::Ptr param);
        // 0x91
        int sched_getscheduler(pid_t pid);
        // 0x95
        int mlock(x64::Ptr addr, size_t len);
        // 0x9e
        int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
        // 0x9e
        int arch_prctl(int code, x64::Ptr addr);
        // 0xba
        int gettid();
        // 0xbf
        ssize_t getxattr(x64::Ptr path, x64::Ptr name, x64::Ptr value, size_t size);
        // 0xc0
        ssize_t lgetxattr(x64::Ptr path, x64::Ptr name, x64::Ptr value, size_t size);
        // 0xc9
        time_t time(x64::Ptr tloc);
        // 0xca
        long futex(x64::Ptr32 uaddr, int futex_op, uint32_t val, x64::Ptr timeout, x64::Ptr32 uaddr2, uint32_t val3);
        // 0xcb
        int sched_setaffinity(pid_t pid, size_t cpusetsize, x64::Ptr mask);
        // 0xcc
        int sched_getaffinity(pid_t pid, size_t cpusetsize, x64::Ptr mask);
        // 0xd9
        ssize_t getdents64(int fd, x64::Ptr dirp, size_t count);
        // 0xda
        pid_t set_tid_address(x64::Ptr32 tidptr);
        // 0xdd
        int posix_fadvise(int fd, off_t offset, off_t len, int advice);
        // 0xe4
        int clock_gettime(clockid_t clockid, x64::Ptr tp);
        // 0xe5
        int clock_getres(clockid_t clockid, x64::Ptr res);
        // 0xe6
        int clock_nanosleep(clockid_t clockid, int flags, x64::Ptr request, x64::Ptr remain);
        // 0xe7
        u64 exit_group(int status);
        // 0xe8
        int epoll_wait(int epfd, x64::Ptr events, int maxevents, int timeout);
        // 0xe9
        int epoll_ctl(int epfd, int op, int fd, x64::Ptr event);
        // 0xea
        int tgkill(int tgid, int tid, int sig);
        // 0xed
        int mbind(unsigned long start, unsigned long len, unsigned long mode, x64::Ptr64 nmask, unsigned long maxnode, unsigned flags);
        // 0xf7
        int waitid(int idtype, id_t id, x64::Ptr infop, int options, x64::Ptr rusage);
        // 0xfd
        int inotify_init();
        // 0xfe
        int inotify_add_watch(int fd, x64::Ptr pathname, uint32_t mask);
        // 0x101
        int openat(int dirfd, x64::Ptr pathname, int flags, mode_t mode);
        // 0x106
        int fstatat64(int dirfd, x64::Ptr pathname, x64::Ptr statbuf, int flags);
        // 0x109
        int linkat(int olddirfd, x64::Ptr oldpath, int newdirfd, x64::Ptr newpath, int flags);
        // 0x10b
        ssize_t readlinkat(int dirfd, x64::Ptr pathname, x64::Ptr buf, size_t bufsiz);
        // 0x10d
        int faccessat(int dirfd, x64::Ptr pathname, int mode);
        // 0x10e
        int pselect6(int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout, x64::Ptr sigmask);
        // 0x10f
        int ppoll(x64::Ptr fds, int nfds, x64::Ptr tmo_p, x64::Ptr sigmask, size_t sigsetsize);
        // 0x111
        long set_robust_list(x64::Ptr head, size_t len);
        // 0x112
        long get_robust_list(int pid, x64::Ptr64 head_ptr, x64::Ptr64 len_ptr);
        // 0x118
        int utimensat(int dirfd, x64::Ptr pathname, x64::Ptr times, int flags);
        // 0x11d
        int fallocate(int fd, int mode, off_t offset, off_t len);
        // 0x122
        int eventfd2(unsigned int initval, int flags);
        // 0x123
        int epoll_create1(int flags);
        // 0x124
        int dup3(int oldfd, int newfd, int flags);
        // 0x125
        int pipe2(x64::Ptr32 pipefd, int flags);
        // 0x126
        int inotify_init1(int flags);
        // 0x12e
        int prlimit64(pid_t pid, int resource, x64::Ptr new_limit, x64::Ptr old_limit);
        // 0x13a
        int sched_setattr(pid_t pid, x64::Ptr attr, unsigned int flags);
        // 0x13b
        int sched_getattr(pid_t pid, x64::Ptr attr, unsigned int size, unsigned int flags);
        // 0x13e
        ssize_t getrandom(x64::Ptr buf, size_t len, int flags);
        // 0x13f
        int memfd_create(x64::Ptr name, unsigned int flags);
        // 0x14c
        int statx(int dirfd, x64::Ptr pathname, int flags, unsigned int mask, x64::Ptr statxbuf);
        // 0x1b3
        int clone3(x64::Ptr uargs, size_t size);

    private:

        template<typename... Args>
        void print(const char* format, Args... args) const;

        Kernel& kernel_;
        x64::Mmu& mmu_;
        std::mutex mutex_;
        Thread* currentThread_ { nullptr };
    };

}

#endif