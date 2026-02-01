#include "kernel/linux/dev/shadowdevice.h"
#include "kernel/linux/dev/nulldevice.h"
#include "kernel/linux/dev/tty.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel::gnulinux {

    std::vector<std::string> ShadowDevice::allAllowedDevices_ { // NOLINT(cert-err58-cpp)
        "/dev/null",
        "/dev/tty",
    };

    std::unique_ptr<Device> ShadowDevice::tryCreate(const Path& path, bool closeOnExec) {
        std::string pathname = path.absolute();

        if(pathname == "/dev/null") {
            return NullDevice::tryCreate(path);
        }
        if(pathname == "/dev/tty") {
            return Tty::tryCreate(path, closeOnExec);
        }

        warn(fmt::format("Device {} is not a supported shadow device", pathname));
        return nullptr;
    }

    std::optional<int> ShadowDevice::tryGetDeviceHostFd(const std::string& pathname, bool closeOnExec) {
        int flags = O_RDWR;
        if(closeOnExec) flags |= O_CLOEXEC;
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

        guard.disable();

        return fd;
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

    ReadResult ShadowDevice::read(OpenFileDescription&, size_t) {
        verify(false, "ShadowDevice::read not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ssize_t ShadowDevice::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "ShadowDevice::write not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer ShadowDevice::stat() {
        return Host::stat(path().absolute());
    }

    ErrnoOrBuffer ShadowDevice::statfs() {
        verify(false, "ShadowDevice::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ShadowDevice::statx(unsigned int) {
        verify(false, "ShadowDevice::statx not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    off_t ShadowDevice::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "ShadowDevice::lseek not implemented");
        return -ENOTSUP;
    }

    std::optional<int> ShadowDevice::fcntl(int, int) {
        verify(false, "ShadowDevice::fcntl not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer ShadowDevice::ioctl(OpenFileDescription&, Ioctl request, const Buffer& inputBuffer) {
        if(!hostFd_) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        switch(request) {
            case Ioctl::tcgets: {
                struct termios ts;
                int ret = ::ioctl(hostFd_.value(), TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                Buffer buffer(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocgpgrp: {
                assert(inputBuffer.size() == sizeof(pid_t));
                pid_t pid;
                std::memcpy(&pid, inputBuffer.data(), sizeof(pid));
                int ret = ::ioctl(hostFd_.value(), TIOCGPGRP, &pid);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            default: break;
        }
        verify(false, fmt::format("ShadowDevice::ioctl({:#x}) not implemented", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}