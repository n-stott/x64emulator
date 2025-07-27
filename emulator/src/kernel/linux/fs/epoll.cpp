#include "kernel/linux/fs/epoll.h"
#include "verify.h"
#include <algorithm>
#include <sys/errno.h>

namespace kernel::gnulinux {

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

    off_t Epoll::lseek(OpenFileDescription&, off_t, int) {
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

    std::optional<int> Epoll::fcntl(int cmd, int arg) {
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on epoll", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Epoll::ioctl(OpenFileDescription&, Ioctl request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on epoll", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }


    ErrnoOr<void> Epoll::addEntry(i32 fd, u32 event, u64 data) {
        verify(std::none_of(interestList_.begin(), interestList_.end(), [&](const Entry& entry) {
            return entry.fd == fd;
        }), "Epoll::addEntry: fd already exists in interest list");
        interestList_.push_back(Entry{
            fd,
            event,
            data,
        });
        return ErrnoOr<void>({});
    }

    ErrnoOr<void> Epoll::changeEntry(i32 fd, u32 event, u64 data) {
        auto it = std::find_if(interestList_.begin(), interestList_.end(), [&](const Entry& entry) {
            return entry.fd == fd;
        });
        verify(it != interestList_.end(), "Epoll:;changeEntry: fd not present in interest list");
        it->event = event;
        it->data = data;
        return ErrnoOr<void>({});
    }

    ErrnoOr<void> Epoll::deleteEntry(i32 fd) {
        verify(std::any_of(interestList_.begin(), interestList_.end(), [&](const Entry& entry) {
            return entry.fd == fd;
        }), "Epoll::deleteEntry: fd not present in interest list");
        interestList_.erase(std::remove_if(interestList_.begin(), interestList_.end(), [&](const Entry& entry) {
            return entry.fd == fd;
        }), interestList_.end());
        return ErrnoOr<void>({});
    }


}