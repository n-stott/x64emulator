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

    std::unique_ptr<Tty> Tty::tryCreate(const Path& path, bool closeOnExec) {
        std::string pathname = path.absolute();
        auto hostFd = ShadowDevice::tryGetDeviceHostFd(pathname, closeOnExec);
        if(!hostFd) return nullptr;
        return std::unique_ptr<Tty>(new Tty(path.last(), hostFd));
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
        return Host::pollCanRead(Host::FD{hostFd_.value()});
    }

    ErrnoOrBuffer Tty::read(OpenFileDescription&, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EBADF};
        if(!hostFd_) return ErrnoOrBuffer{-EBADF};
        Buffer buffer(count, 0x0);
        ssize_t nbytes = ::read(hostFd_.value(), buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
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
        Buffer buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(std::move(buf));
    }

    void Tty::advanceInternalOffset(off_t) {
        // nothing to do here
    }

    off_t Tty::lseek(OpenFileDescription&, off_t, int) {
        return -ESPIPE;
    }

    std::optional<int> Tty::fcntl(int cmd, int arg) {
        if(!hostFd_) return -EBADF;
        int ret = ::fcntl(hostFd_.value(), cmd, arg);
        return ret;
    }

    ErrnoOrBuffer Tty::ioctl(OpenFileDescription&, Ioctl request, const Buffer& inputBuffer) {
        if(!hostFd_) {
            verify(false, "ShadowDevice without host backer is not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        Buffer buffer(inputBuffer);
        switch(request) {
            case Ioctl::tcgets: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCGETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tcsets: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCSETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::fioclex: {
                int ret = ::ioctl(hostFd_.value(), FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::fionclex: {
                int ret = ::ioctl(hostFd_.value(), FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::fionbio: {
                int ret = ::ioctl(hostFd_.value(), FIONBIO, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocgwinsz: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl(hostFd_.value(), TIOCGWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocswinsz: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl(hostFd_.value(), TIOCSWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tcsetsw: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl(hostFd_.value(), TCSETSW, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocgpgrp: {
                verify(buffer.size() == sizeof(pid_t));
                int ret = ::ioctl(hostFd_.value(), TIOCGPGRP, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            default: break;
        }
        verify(false, fmt::format("Tty::ioctl({:#x}) not implemented", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}