#include "kernel/fs/epoll.h"
#include <sys/errno.h>

namespace kernel {

    void Epoll::close() {
        (void)flags_;
    }
    
    ErrnoOrBuffer Epoll::read(size_t, off_t) {
        return ErrnoOrBuffer(-EINVAL);
    }
    
    ssize_t Epoll::write(const u8*, size_t, off_t) {
        return -EINVAL;
    }

    off_t Epoll::lseek(off_t, int) {
        return -ESPIPE;
    }

}