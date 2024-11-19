#include "kernel/dev/tty.h"
#include "kernel/fs/path.h"
#include "scopeguard.h"
#include "verify.h"
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    Tty* Tty::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
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

        auto tty = std::unique_ptr<Tty>(new Tty(fs, containingDirectory, path->last(), fd));
        return containingDirectory->addFile(std::move(tty));
    }

    void Tty::close() {
        if(refCount_ > 0) return;
        if(!!hostFd_) {
            int rc = ::close(hostFd_.value());
            verify(rc == 0);
        }
    }

    bool Tty::canRead() const {
        if(!isPollable()) return false;
        if(!hostFd_) return false;
        struct pollfd pfd;
        pfd.fd = hostFd_.value();
        pfd.events = POLLIN;
        pfd.revents = 0;
        int timeout = 0; // return immediately
        int ret = ::poll(&pfd, 1, timeout);
        if(ret < 0) return false;
        return !!(pfd.revents & POLLIN);
    }

    ErrnoOrBuffer Tty::read(OpenFileDescription&, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EBADF};
        if(!hostFd_) return ErrnoOrBuffer{-EBADF};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(hostFd_.value(), buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t Tty::write(OpenFileDescription&, const u8* buf, size_t count) {
        if(!isReadable()) return -EBADF;
        if(!hostFd_) return -EBADF;
        ssize_t ret = ::write(2, buf, count);
        return ret;
    }

    ErrnoOrBuffer Tty::stat() {
        if(!hostFd_) return ErrnoOrBuffer(-EBADF);
        struct stat st;
        int rc = ::fstat(hostFd_.value(), &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        std::vector<u8> buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }

    off_t Tty::lseek(OpenFileDescription&, off_t, int) {
        return -ESPIPE;
    }

    std::optional<int> Tty::fcntl(int, int) {
        verify(false, "Tty::fcntl not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer Tty::ioctl(unsigned long request, const Buffer& inputBuffer) {
        if(!hostFd_) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        Buffer buffer(inputBuffer);
        switch(request) {
            case TCGETS: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCGETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TCSETS: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCSETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case FIOCLEX: {
                int ret = ::ioctl(hostFd_.value(), FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case FIONCLEX: {
                int ret = ::ioctl(hostFd_.value(), FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case TIOCGWINSZ: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl(hostFd_.value(), TIOCGWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TIOCSWINSZ: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl(hostFd_.value(), TIOCSWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TCSETSW: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCSETSW, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TIOCGPGRP: {
                verify(buffer.size() == sizeof(pid_t));
                int ret = ::ioctl(hostFd_.value(), TIOCGPGRP, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            default: break;
        }
        verify(false, fmt::format("Tty::ioctl({:#x}) not implemented", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}