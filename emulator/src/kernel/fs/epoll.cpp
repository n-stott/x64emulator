#include "kernel/fs/epoll.h"
#include "verify.h"
#include <sys/errno.h>

namespace kernel {

    void Epoll::close() {
        (void)flags_;
    }
    
    ErrnoOrBuffer Epoll::read(OpenFileDescription&, size_t) {
        verify(false, "Epoll::read not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ssize_t Epoll::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "Epoll::write not implemented");
        return -ENOTSUP;
    }

    off_t Epoll::lseek(off_t, int) {
        return -ESPIPE;
    }

    ErrnoOrBuffer Epoll::stat() {
        verify(false, "stat not implemented on epoll");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Epoll::statfs() {
        verify(false, "statfs not implemented on epoll");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer Epoll::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

    int Epoll::fcntl(int cmd, int arg) {
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on epoll", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Epoll::ioctl(unsigned long request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on epoll", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}