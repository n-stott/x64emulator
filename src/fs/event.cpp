#include "fs/event.h"
#include <sys/errno.h>

namespace kernel {

    void Event::close() {

    }
    
    ErrnoOrBuffer Event::read(size_t) {
        return ErrnoOrBuffer(-EINVAL);
    }
    
    ssize_t Event::write(const u8*, size_t) {
        return -EINVAL;
    }

}