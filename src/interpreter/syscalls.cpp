#include "interpreter/syscalls.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"
#include <sys/stat.h>

namespace x64 {

    u64 Sys::fstat(unsigned int fd, Ptr8 statbuf) {
        struct stat st;
        int rc = ::fstat(fd, &st);
        u8 buf[sizeof(st)];
        memcpy(&buf, &st, sizeof(st));
        mmu_->copyToMmu(statbuf, buf, sizeof(buf));
        return rc;
    }

}