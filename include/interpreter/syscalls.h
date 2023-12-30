#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"
#include "utils/utils.h"

namespace x64 {

    class VM;
    class Mmu;

    class Sys {
    public:
        Sys(VM* vm, Mmu* mmu) : vm_(vm), mmu_(mmu) { }

        // 0x0
        ssize_t read(int fd, Ptr8 buf, size_t count);
        // 0x1
        ssize_t write(int fd, Ptr8 buf, size_t count);
        // 0x3
        int close(int fd);
        // 0x5
        int fstat(int fd, Ptr8 statbuf);
        // 0x9
        u64 mmap(u64 addr, size_t length, int prot, int flags, int fd, off_t offset);
        // 0xa
        int mprotect(u64 addr, size_t length, int prot);
        // 0xb
        int munmap(u64 addr, size_t length);
        // 0xc
        u64 brk(u64 addr);
        // 0x3f
        int uname(u64 buf);
        // 04f
        u64 getcwd(u64 buf, size_t size);
        // 0x59
        ssize_t readlink(u64 pathname, u64 buf, size_t bufsiz);
        // 0x9e
        void exit_group(int status);
        // 0xe7
        int arch_prctl(int code, u64 addr);
        // 0x101
        int openat(int dirfd, u64 pathname, int flags, mode_t mode);


    private:
        VM* vm_;
        Mmu* mmu_;
    };

}

#endif