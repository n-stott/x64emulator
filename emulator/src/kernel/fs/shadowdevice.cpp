#include "kernel/fs/shadowdevice.h"
#include "kernel/fs/path.h"
#include "scopeguard.h"
#include "verify.h"
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    File* ShadowDevice::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        if(pathname != "/dev/tty") {
            warn("/dev/tty is the only ShadowDevice that is supported");
            return {};
        }

        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
        if(fd < 0) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return {};
        }

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

        auto hostFile = std::unique_ptr<ShadowDevice>(new ShadowDevice(fs, containingDirectory, path->last(), fd));
        return containingDirectory->addFile(std::move(hostFile));
    }

    void ShadowDevice::close() {
        if(refCount_ > 0) return;
        if(!!hostFd_) {
            int rc = ::close(hostFd_.value());
            verify(rc == 0);
        }
    }

    bool ShadowDevice::canRead() const {
        verify(false, "ShadowDevice::canRead not implemented");
        return false;
    }

    bool ShadowDevice::canWrite() const {
        verify(false, "ShadowDevice::canWrite not implemented");
        return false;
    }

    ErrnoOrBuffer ShadowDevice::read(size_t, off_t) {
        verify(false, "ShadowDevice::read not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ssize_t ShadowDevice::write(const u8*, size_t, off_t) {
        verify(false, "ShadowDevice::write not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer ShadowDevice::stat() {
        verify(false, "ShadowDevice::stat not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ShadowDevice::statfs() {
        verify(false, "ShadowDevice::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    off_t ShadowDevice::lseek(off_t, int) {
        verify(false, "ShadowDevice::lseek not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer ShadowDevice::getdents64(size_t) {
        verify(false, "ShadowDevice::getdents64 not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    int ShadowDevice::fcntl(int, int) {
        verify(false, "ShadowDevice::fcntl not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer ShadowDevice::ioctl(unsigned long request, const Buffer& inputBuffer) {
        if(!hostFd_) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        switch(request) {
            case TCGETS: {
                struct termios ts;
                int ret = ::ioctl(hostFd_.value(), TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                std::vector<u8> buffer;
                buffer.resize(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(Buffer{std::move(buffer)});
            }
            case TIOCGPGRP: {
                assert(inputBuffer.size() == sizeof(pid_t));
                pid_t pid;
                std::memcpy(&pid, inputBuffer.data(), sizeof(pid));
                int ret = ::ioctl(hostFd_.value(), TIOCGPGRP, &pid);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            default: break;
        }
        verify(false, fmt::format("ShadowDevice::ioctl({:#x}) not implemented", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}