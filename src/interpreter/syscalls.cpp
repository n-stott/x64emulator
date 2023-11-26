#include "interpreter/syscalls.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"
#include <sys/stat.h>

namespace x64 {

    u64 Sys::fstat(int fd, Ptr8 statbuf) {
        (void)interpreter_;
        struct stat st;
        int rc = ::fstat(fd, &st);
        if(rc < 0) return (u64)rc;
        u8 buf[sizeof(st)];
        memcpy(&buf, &st, sizeof(st));
        mmu_->copyToMmu(statbuf, buf, sizeof(buf));
        return (u64)rc;
    }

}