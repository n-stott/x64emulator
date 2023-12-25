#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"
#include "utils/utils.h"

namespace x64 {

    class Interpreter;
    class Mmu;

    class Sys {
    public:
        Sys(Interpreter* interpreter, Mmu* mmu) : interpreter_(interpreter), mmu_(mmu) { }

        ssize_t write(int fd, Ptr8 buf, size_t count);
        int fstat(int fd, Ptr8 statbuf);
        u64 mmap(u64 addr, size_t length, int prot, int flags, int fd, off_t offset);
        int mprotect(u64 addr, size_t length, int prot);
        int munmap(u64 addr, size_t length);
        int uname(u64 buf);
        ssize_t readlink(u64 pathname, u64 buf, size_t bufsiz);
        u64 brk(u64 addr);
        void exit_group(int status);
        int arch_prctl(int code, u64 addr);


    private:
        Interpreter* interpreter_;
        Mmu* mmu_;
    };

}

#endif