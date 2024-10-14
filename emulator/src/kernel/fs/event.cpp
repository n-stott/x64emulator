#include "kernel/fs/event.h"
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
            initval_(initval),
            flags_(flags),
            hostFd_(hostFd) {
        (void)initval_;
        (void)flags_;
    }

    void Event::close() {

    }
    
    ErrnoOrBuffer Event::read(size_t, off_t) {
        return ErrnoOrBuffer(-EINVAL);
    }
    
    ssize_t Event::write(const u8*, size_t, off_t) {
        return -EINVAL;
    }

}