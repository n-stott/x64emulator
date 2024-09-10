#include "kernel/fs/epoll.h"
#include <sys/errno.h>

namespace kernel {

    void Epoll::close() {

    }
    
    ErrnoOrBuffer Epoll::read(size_t) {
        return ErrnoOrBuffer(-EINVAL);
    }
    
    ssize_t Epoll::write(const u8*, size_t) {
        return -EINVAL;
    }

}