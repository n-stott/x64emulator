#include "fs/hostfile.h"
#include <fcntl.h>
#include <unistd.h>

namespace kernel {

    std::unique_ptr<HostFile> HostFile::tryCreate(FS* fs, const std::string& path) {
        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, path.c_str(), flags);
        if(fd < 0) return {};
        return std::unique_ptr<HostFile>(new HostFile(fs, fd));
    }

    ErrnoOrBuffer HostFile::read(size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(hostFd_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t HostFile::write(const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

    ErrnoOrBuffer HostFile::pread(size_t count, size_t offset) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::pread(hostFd_, buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t HostFile::pwrite(const u8*, size_t, size_t) {
        return -EINVAL;
    }

}