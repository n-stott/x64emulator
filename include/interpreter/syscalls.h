#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"
#include "utils/utils.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <unistd.h>
#include <sys/types.h>

namespace x64 {

    class VM;
    class Mmu;

    class Sys {
    public:
        Sys(VM* vm, Mmu* mmu) : vm_(vm), mmu_(mmu) { }

        void syscall();

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
            if constexpr(std::is_same_v<T, Ptr>) {
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
        ssize_t read(int fd, Ptr buf, size_t count);
        // 0x1
        ssize_t write(int fd, Ptr buf, size_t count);
        // 0x3
        int close(int fd);
        // 0x3
        int stat(Ptr pathname, Ptr statbuf);
        // 0x5
        int fstat(int fd, Ptr statbuf);
        // 0x6
        int lstat(Ptr pathname, Ptr statbuf);
        // 0x7
        int poll(Ptr fds, size_t nfds, int timeout);
        // 0x8
        off_t lseek(int fd, off_t offset, int whence);
        // 0x9
        Ptr mmap(Ptr addr, size_t length, int prot, int flags, int fd, off_t offset);
        // 0xa
        int mprotect(Ptr addr, size_t length, int prot);
        // 0xb
        int munmap(Ptr addr, size_t length);
        // 0xc
        Ptr brk(Ptr addr);
        // 0xd
        int rt_sigaction(int sig, Ptr act, Ptr oact, size_t sigsetsize);
        // 0xe
        int rt_sigprocmask(int how, Ptr nset, Ptr oset, size_t sigsetsize);
        // 0x10
        int ioctl(int fd, unsigned long request, Ptr argp);
        // 0x11
        ssize_t pread64(int fd, Ptr buf, size_t count, off_t offset);
        // 0x14
        ssize_t writev(int fd, Ptr iov, int iovcnt);
        // 0x15
        int access(Ptr pathname, int mode);
        // 0x17
        int select(int nfds, Ptr readfds, Ptr writefds, Ptr exceptfds, Ptr timeout);
        // 0x20
        int dup(int oldfd);
        // 0x26
        int setitimer(int which, const Ptr new_value, Ptr old_value);
        // 0x27
        int getpid();
        // 0x29
        int socket(int domain, int type, int protocol);
        // 0x2a
        int connect(int sockfd, Ptr addr, size_t addrlen);
        // 0x2d
        ssize_t recvfrom(int sockfd, Ptr buf, size_t len, int flags, Ptr src_addr, Ptr32 addrlen);
        // 0x2f
        ssize_t recvmsg(int sockfd, Ptr msg, int flags);
        // 0x33
        int getsockname(int sockfd, Ptr addr, Ptr32 addrlen);
        // 0x34
        int getpeername(int sockfd, Ptr addr, Ptr32 addrlen);
        // 0x3f
        int uname(Ptr buf);
        // 0x48
        int fcntl(int fd, int cmd, int arg);
        // 0x4a
        int fsync(int fd);
        // 0x4f
        Ptr getcwd(Ptr buf, size_t size);
        // 0x50
        int chdir(Ptr path);
        // 0x53
        int mkdir(Ptr pathname, mode_t mode);
        // 0x57
        int unlink(Ptr pathname);
        // 0x59
        ssize_t readlink(Ptr pathname, Ptr buf, size_t bufsiz);
        // 0x60
        int gettimeofday(Ptr tv, Ptr tz);
        // 0x63
        int sysinfo(Ptr info);
        // 0x66
        int getuid();
        // 0x68
        int getgid();
        // 0x6b
        int geteuid();
        // 0x6c
        int getegid();
        // 0x89
        int statfs(Ptr path, Ptr buf);
        // 0x9e
        int arch_prctl(int code, Ptr addr);
        // 0xbf
        ssize_t getxattr(Ptr path, Ptr name, Ptr value, size_t size);
        // 0xc0
        ssize_t lgetxattr(Ptr path, Ptr name, Ptr value, size_t size);
        // 0xc9
        time_t time(Ptr tloc);
        // 0xca
        long futex(Ptr32 uaddr, int futex_op, uint32_t val, Ptr timeout, Ptr32 uaddr2, uint32_t val3);
        // 0xd9
        ssize_t getdents64(int fd, Ptr dirp, size_t count);
        // 0xda
        pid_t set_tid_address(Ptr32 tidptr);
        // 0xdd
        int posix_fadvise(int fd, off_t offset, off_t len, int advice);
        // 0xe4
        int clock_gettime(clockid_t clockid, Ptr tp);
        // 0xe7
        u64 exit_group(int status);
        // 0xea
        int tgkill(int tgid, int tid, int sig);
        // 0x101
        int openat(int dirfd, Ptr pathname, int flags, mode_t mode);
        // 0x10e
        int pselect6(int nfds, Ptr readfds, Ptr writefds, Ptr exceptfds, Ptr timeout, Ptr sigmask);
        // 0x111
        long set_robust_list(Ptr head, size_t len);
        // 0x112
        long get_robust_list(int pid, Ptr64 head_ptr, Ptr64 len_ptr);
        // 0x118
        int utimensat(int dirfd, Ptr pathname, Ptr times, int flags);
        // 0x12e
        int prlimit64(pid_t pid, int resource, Ptr new_limit, Ptr old_limit);
        // 0x13e
        ssize_t getrandom(Ptr buf, size_t len, int flags);
        // 0x14c
        int statx(int dirfd, Ptr pathname, int flags, unsigned int mask, Ptr statxbuf);


    private:
        VM* vm_;
        Mmu* mmu_;
    };

}

#endif