#include "kernel/fs/event.h"
#include "verify.h"
#include <sys/errno.h>
#include <sys/eventfd.h>

namespace kernel {

    std::unique_ptr<Event> Event::tryCreate(FS* fs, unsigned int initval, int flags) {
        int fd = ::eventfd(initval, flags);
        if(fd < 0) return {};
        return std::unique_ptr<Event>(new Event(fs, initval, flags, fd));
    }

    Event::Event(FS* fs, unsigned int initval, int flags, int hostFd) :
            File(fs),
            counter_(initval),
            flags_(flags),
            hostFd_(hostFd) {
        (void)counter_;
        (void)flags_;
    }

    void Event::close() {

    }
    
    ErrnoOrBuffer Event::read(size_t, off_t) {
        return ErrnoOrBuffer(-EINVAL);
    }
    
    ssize_t Event::write(const u8* buf, size_t size, off_t offset) {
        verify(offset == 0);
        if(size < 8) return -EINVAL;
        u64 value;
        std::memcpy(&value, buf, sizeof(value));
        if(value == (u64)(-1)) return -EINVAL;
        verify(value < (u64)(-2) - counter_);
        counter_ += value;
        return sizeof(value);
    }

    off_t Event::lseek(off_t, int) {
        return -ESPIPE;
    }

}