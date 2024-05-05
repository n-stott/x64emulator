#include "fs/stream.h"
#include <sys/errno.h>
#include <unistd.h>

namespace kernel {

    bool Stream::isReadable() const {
        return type_ == TYPE::IN;
    }

    bool Stream::isWritable() const {
        return type_ == TYPE::OUT || type_ == TYPE::ERR;
    }

    ssize_t Stream::read(u8* buf, size_t count) {
        if(!isReadable()) return -EINVAL;
        return ::read((int)type_, buf, count);
        
    }

    ssize_t Stream::write(const u8* buf, size_t count) {
        if(!isWritable()) return -EINVAL;
        return ::write((int)type_, buf, count);
    }

}