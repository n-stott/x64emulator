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
        // 0x9e
        void exit_group(int status);
        // 0xe7
        int arch_prctl(int code, Ptr addr);
        // 0x101
        int openat(int dirfd, Ptr pathname, int flags, mode_t mode);


    private:
        VM* vm_;
        Mmu* mmu_;
    };

}

#endif