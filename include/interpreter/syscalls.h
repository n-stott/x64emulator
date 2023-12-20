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

        void exit_group(int status);
        u64 fstat(int fd, Ptr8 statbuf);

    private:
        Interpreter* interpreter_;
        Mmu* mmu_;
    };

}

#endif