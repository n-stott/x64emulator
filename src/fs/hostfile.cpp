#include "fs/hostfile.h"
#include "interpreter/verify.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    std::unique_ptr<HostFile> HostFile::tryCreate(FS* fs, const std::string& path) {
        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, path.c_str(), flags);
        if(fd < 0) return {};
        return std::unique_ptr<HostFile>(new HostFile(fs, path, fd));
    }

    void HostFile::close() {
        int rc = ::close(hostFd_);
        x64::verify(rc == 0);
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

    ErrnoOrBuffer HostFile::pread(size_t count, off_t offset) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::pread(hostFd_, buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t HostFile::pwrite(const u8*, size_t, off_t) {
        return -EINVAL;
    }

    ErrnoOrBuffer HostFile::stat() {
        struct stat st;
        int rc = ::stat(path_.c_str(), &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        std::vector<u8> buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }

    off_t HostFile::lseek(off_t offset, int whence) {
        off_t ret = ::lseek(hostFd_, offset, whence);
        if(ret < 0) return -errno;
        return ret;
    }

}