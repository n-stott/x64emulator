#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"
#include "utils/utils.h"
#include <cstdint>

namespace x64 {

    class VM;
    class Mmu;

    class Sys {
    public:
        Sys(VM* vm, Mmu* mmu) : vm_(vm), mmu_(mmu) { }

        void syscall();

    private:
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
        //0x10
        int ioctl(int fd, unsigned long request, Ptr argp);
        // 0x14
        ssize_t writev(int fd, Ptr iov, int iovcnt);
        // 0x15
        int access(Ptr pathname, int mode);
        // 0x27
        int getpid();
        // 0x3f
        int uname(Ptr buf);
        // 04f
        Ptr getcwd(Ptr buf, size_t size);
        // 0x59
        ssize_t readlink(Ptr pathname, Ptr buf, size_t bufsiz);
        // 0x63
        int sysinfo(Ptr info);
        // 0x9e
        void exit_group(int status);
        // 0xca
        long futex(Ptr32 uaddr, int futex_op, uint32_t val, Ptr timeout, Ptr32 uaddr2, uint32_t val3);
        // 0xda
        pid_t set_tid_address(Ptr32 tidptr);
        // 0xe4
        int clock_gettime(clockid_t clockid, Ptr tp);
        // 0xe7
        int arch_prctl(int code, Ptr addr);
        // 0x101
        int openat(int dirfd, Ptr pathname, int flags, mode_t mode);
        // 0x111
        long set_robust_list(Ptr head, size_t len);
        // 0x112
        long get_robust_list(int pid, Ptr64 head_ptr, Ptr64 len_ptr);
        // 0x12e
        int prlimit64(pid_t pid, int resource, Ptr new_limit, Ptr old_limit);
        // 0x13e
        ssize_t getrandom(Ptr buf, size_t len, int flags);


    private:
        VM* vm_;
        Mmu* mmu_;
    };

}

#endif