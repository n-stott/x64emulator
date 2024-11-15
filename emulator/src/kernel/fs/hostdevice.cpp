#include "kernel/fs/hostdevice.h"
#include "kernel/fs/path.h"
#include "kernel/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    File* HostDevice::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
        if(fd < 0) return {};

        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        // check that the file is a device
        struct stat s;
        if(::fstat(fd, &s) < 0) {
            return {};
        }
        
        mode_t fileType = (s.st_mode & S_IFMT);
        if(fileType != S_IFCHR && fileType != S_IFBLK) {
            // not a character device or block device
            return {};
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        guard.disable();

        auto hostDevice = std::unique_ptr<HostDevice>(new HostDevice(fs, containingDirectory, path->last(), fd));
        return containingDirectory->addFile(std::move(hostDevice));
    }

    void HostDevice::close() {
        if(refCount_ > 0) return;
        int rc = ::close(hostFd_);
        verify(rc == 0);
    }

    bool HostDevice::canRead() const {
        verify(false, "HostDevice::canRead not implemented");
        return false;
    }

    bool HostDevice::canWrite() const {
        verify(false, "HostDevice::canWrite not implemented");
        return false;
    }

    ErrnoOrBuffer HostDevice::read(OpenFileDescription&, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(hostFd_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t HostDevice::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "HostDevice::write not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer HostDevice::stat() {
        verify(false, "HostDevice::stat not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer HostDevice::statfs() {
        verify(false, "HostDevice::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    off_t HostDevice::lseek(off_t, int) {
        verify(false, "HostDevice::lseek not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer HostDevice::getdents64(size_t) {
        verify(false, "HostDevice::getdents64 not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    std::optional<int> HostDevice::fcntl(int cmd, int arg) {
        return Host::fcntl(Host::FD{hostFd_}, cmd, arg);
    }

    ErrnoOrBuffer HostDevice::ioctl(unsigned long, const Buffer&) {
        verify(false, "HostDevice::ioctl not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

}