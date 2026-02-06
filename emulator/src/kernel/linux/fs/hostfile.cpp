#include "kernel/linux/fs/hostfile.h"
#include "kernel/linux/fs/openfiledescription.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <fmt/color.h>
#include <asm/termbits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace kernel::gnulinux {

    std::unique_ptr<HostFile> HostFile::tryCreate(const Path& path, BitFlags<AccessMode> accessMode, bool closeOnExec) {
        std::string pathname = path.absolute();

        verify(!accessMode.test(AccessMode::WRITE), "HostFile should not have write access");
        int flags = O_RDONLY;
        if(closeOnExec) flags |= O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
        if(fd < 0) return {};

        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        // check that the file is a regular file
        struct stat s;
        if(::fstat(fd, &s) < 0) {
            return {};
        }
        
        mode_t fileType = (s.st_mode & S_IFMT);
        if (fileType != S_IFREG) {
            // not a regular file
            return {};
        }
        guard.disable();

        return std::unique_ptr<HostFile>(new HostFile(path.last(), fd));
    }

    void HostFile::close() {
        if(refCount_ > 0) return;
        int rc = ::close(hostFd_);
        verify(rc == 0);
    }

    bool HostFile::canRead() const {
        return Host::pollCanRead(Host::FD{hostFd_});
    }

    bool HostFile::canWrite() const {
        verify(false, "HostFile::canWrite not implemented");
        return false;
    }

    ReadResult HostFile::read(OpenFileDescription& openFileDescription, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        off_t offset = openFileDescription.offset();
        if(offset < 0) return ErrnoOrBuffer{-EINVAL};
        Buffer buffer(count, 0x0);
        ssize_t nbytes = ::pread(hostFd_, buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ssize_t HostFile::write(OpenFileDescription&, const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

    ErrnoOrBuffer HostFile::stat() {
        struct stat st;
        std::string path = this->path().absolute();
        int rc = ::stat(path.c_str(), &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        Buffer buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(std::move(buf));
    }

    ErrnoOrBuffer HostFile::statfs() {
        struct statfs stfs;
        int rc = ::fstatfs(hostFd_, &stfs);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        Buffer buf(sizeof(stfs), 0x0);
        std::memcpy(buf.data(), &stfs, sizeof(stfs));
        return ErrnoOrBuffer(std::move(buf));
    }

    ErrnoOrBuffer HostFile::statx(unsigned int mask) {
        return Host::statx(Host::FD{hostFd_}, "", AT_EMPTY_PATH, mask);
    }

    void HostFile::advanceInternalOffset(off_t offset) {
        off_t ret = ::lseek(hostFd_, offset, SEEK_CUR);
        verify(ret >= 0, []() {
            fmt::print("Expected no error in HostFile::advanceInternalOffset, but got errno = {}\n", errno);
        });
    }

    off_t HostFile::lseek(OpenFileDescription& ofd, off_t offset, int whence) {
        off_t ret = [&]() {
            if(whence == SEEK_CUR) {
                return ::lseek(hostFd_, ofd.offset() + offset, SEEK_SET);
            } else {
                // SEEK_SET or SEEK_END
                return ::lseek(hostFd_, offset, whence);
            }
        }();
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer HostFile::getdents64(size_t count) {
        Buffer buf(count, 0x0);
        ssize_t nbytes = ::getdents64(hostFd_, buf.data(), buf.size());
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buf.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buf));
    }

    std::optional<int> HostFile::fcntl(int cmd, int arg) {
        return Host::fcntl(Host::FD{hostFd_}, cmd, arg);
    }

    ErrnoOrBuffer HostFile::ioctl(OpenFileDescription&, Ioctl request, const Buffer& inputBuffer) {
        switch(request) {
            case Ioctl::tcgets: {
                struct termios ts;
                int ret = ::ioctl(hostFd_, TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                Buffer buffer(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::fioclex: {
                int ret = ::ioctl(hostFd_, FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::fionclex: {
                int ret = ::ioctl(hostFd_, FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::tiocgwinsz: {
                struct winsize ws;
                int ret = ::ioctl(hostFd_, TIOCGWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                Buffer buffer(sizeof(ws), 0x0);
                std::memcpy(buffer.data(), &ws, sizeof(ws));
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocswinsz: {
                struct winsize ws;
                std::memcpy(&ws, inputBuffer.data(), sizeof(ws));
                int ret = ::ioctl(hostFd_, TIOCSWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::tcsetsw: {
                struct termios ts;
                std::memcpy(&ts, inputBuffer.data(), sizeof(ts));
                int ret = ::ioctl(hostFd_, TCSETSW, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            default: break;
        }
        verify(false, [&]() {
            fmt::print("implement ioctl {:#x} on HostFile\n", (int)request);
        });
        return ErrnoOrBuffer(-ENOTSUP);
    }

}