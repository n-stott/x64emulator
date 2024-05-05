#include "fs/hostfile.h"
#include <fcntl.h>

namespace kernel {

    std::unique_ptr<HostFile> HostFile::tryCreate(FS* fs, const std::string& path) {
        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, path.c_str(), flags);
        if(fd < 0) return {};
        return std::unique_ptr<HostFile>(new HostFile(fs, fd));
    }

    ssize_t HostFile::read(u8* buf, size_t count) {
        if(!isReadable()) return -EINVAL;
        (void)buf;
        (void)count;
        return -1;
    }

    ssize_t HostFile::write(const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

}