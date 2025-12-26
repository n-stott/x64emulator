#include "kernel/linux/fs/event.h"
#include "kernel/linux/fs/openfiledescription.h"
#include "verify.h"
#include <sys/errno.h>
#include <sys/poll.h>

namespace kernel::gnulinux {

    std::unique_ptr<Event> Event::tryCreate(unsigned int initval, int flags) {
        return std::unique_ptr<Event>(new Event(initval, flags));
    }

    Event::Event(unsigned int initval, int flags) :
            counter_(initval),
            flags_(flags) {
        (void)flags_;
    }

    void Event::close() {

    }

    bool Event::canRead() const {
        return counter_ > 0;
    }

    bool Event::canWrite() const {
        return counter_ < (u64)(-2);
    }
    
    ErrnoOrBuffer Event::read(OpenFileDescription& openFileDescription, size_t size) {
        if(size < 8) return ErrnoOrBuffer(-EINVAL);
        if(counter_ == 0) {
            if(openFileDescription.statusFlags().test(FS::StatusFlags::NONBLOCK)) {
                return ErrnoOrBuffer(-EAGAIN);
            } else {
                verify(false, "Blocking is Event::read not supported");
                return ErrnoOrBuffer(-ENOTSUP);
            }
        } else {
            Buffer buffer(counter_);
            verify(buffer.size() == 8);
            counter_ = 0;
            return ErrnoOrBuffer(std::move(buffer));
        }
    }
    
    ssize_t Event::write(OpenFileDescription&, const u8* buf, size_t size) {
        if(size < 8) return -EINVAL;
        u64 value;
        std::memcpy(&value, buf, sizeof(value));
        if(value == (u64)(-1)) return -EINVAL;
        verify(value < (u64)(-2) - counter_);
        counter_ += value;
        return sizeof(value);
    }

    void Event::advanceInternalOffset(off_t) {
        // nothing to do
    }

    off_t Event::lseek(OpenFileDescription&, off_t, int) {
        return -ESPIPE;
    }

    ErrnoOrBuffer Event::stat() {
        verify(false, "stat not implemented on event");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Event::statfs() {
        verify(false, "statfs not implemented on event");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Event::statx(unsigned int) {
        verify(false, "statx not implemented on event");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer Event::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

    std::optional<int> Event::fcntl(int cmd, int arg) {
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on event", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Event::ioctl(OpenFileDescription&, Ioctl request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on event", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}