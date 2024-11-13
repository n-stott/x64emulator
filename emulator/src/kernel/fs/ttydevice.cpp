#include "kernel/fs/ttydevice.h"
#include "kernel/fs/path.h"
#include "scopeguard.h"
#include "verify.h"
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    File* TtyDevice::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        int flags = O_RDWR | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
        if(fd < 0) {
            verify(false, "Failed to open /dev/tty");
            return nullptr;
        }

        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        // check that the file is a device
        struct stat s;
        if(::fstat(fd, &s) < 0) {
            return nullptr;
        }
        
        mode_t fileType = (s.st_mode & S_IFMT);
        if(fileType != S_IFCHR && fileType != S_IFBLK) {
            // not a character device or block device
            return nullptr;
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        guard.disable();

        auto ttyDevice = std::unique_ptr<ShadowDevice>(new TtyDevice(fs, containingDirectory, path->last(), fd));
        return containingDirectory->addFile(std::move(ttyDevice));
    }

    void TtyDevice::close() {
        if(refCount_ > 0) return;
        if(!!hostFd_) {
            int rc = ::close(hostFd_.value());
            verify(rc == 0);
        }
    }

    ErrnoOrBuffer TtyDevice::read(size_t, off_t) {
        verify(false, "TtyDevice::read not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ssize_t TtyDevice::write(const u8* buf, size_t count, off_t) {
        if(!!hostFd_) {
            ssize_t ret = ::write(hostFd_.value(), buf, count);
            return ret;
        }
        verify(false, "TtyDevice::write not implemented");
        return -ENOTSUP;
    }

    off_t TtyDevice::lseek(off_t, int) {
        return -ESPIPE;
    }

    ErrnoOrBuffer TtyDevice::getdents64(size_t) {
        verify(false, "TtyDevice::getdents64 not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    int TtyDevice::fcntl(int, int) {
        verify(false, "TtyDevice::fcntl not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer TtyDevice::ioctl(unsigned long request, const Buffer& inputBuffer) {
        if(!hostFd_) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        Buffer buffer(inputBuffer);
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
            case TIOCGWINSZ: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl(hostFd_.value(), TIOCGWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            default: break;
        }
        verify(false, fmt::format("TtyDevice::ioctl({:#x}) not implemented", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}