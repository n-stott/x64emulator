#include "fs/stream.h"
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    bool Stream::isReadable() const {
        return type_ == TYPE::IN;
    }

    bool Stream::isWritable() const {
        return type_ == TYPE::OUT || type_ == TYPE::ERR;
    }

    void Stream::close() {
        
    }

    ErrnoOrBuffer Stream::read(size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read((int)type_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
        
    }

    ssize_t Stream::write(const u8* buf, size_t count) {
        if(!isWritable()) return -EINVAL;
        return ::write((int)type_, buf, count);
    }

    ErrnoOrBuffer Stream::pread(size_t, off_t) {
        // File must be seekable
        return ErrnoOrBuffer{-EINVAL};
    }

    ssize_t Stream::pwrite(const u8*, size_t, off_t) {
        // File must be seekable
        return -EINVAL;
    }

    ErrnoOrBuffer Stream::stat() {
        if(type_ != TYPE::IN
        && type_ != TYPE::OUT
        && type_ != TYPE::ERR) return ErrnoOrBuffer(-EINVAL);
        struct stat st;
        int rc = ::fstat((int)type_, &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        std::vector<u8> buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }

    off_t Stream::lseek(off_t, int) {
        return -ESPIPE;
    }

}