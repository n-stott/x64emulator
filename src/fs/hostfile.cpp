#include "fs/hostfile.h"

namespace kernel {


    ssize_t HostFile::read(u8* buf, size_t count) const {
        (void)buf;
        (void)count;
        return -1;
    }

    ssize_t HostFile::write(const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

}