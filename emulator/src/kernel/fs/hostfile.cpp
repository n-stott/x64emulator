#include "kernel/fs/hostfile.h"
#include "scopeguard.h"
#include "verify.h"
#include <fmt/color.h>
#include <asm/termbits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    std::unique_ptr<HostFile> HostFile::tryCreate(FS* fs, const std::string& path) {
        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, path.c_str(), flags);
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
        if (fileType != S_IFREG && fileType != S_IFLNK && fileType != S_IFDIR) {
            // not a regular file or a symbolic link
            warn([&](){ fmt::print(fg(fmt::color::red), "File {} is not a regular file or a symbolic link\n", path); });
            return {};
        }

        guard.disable();
        return std::unique_ptr<HostFile>(new HostFile(fs, path, fd));
    }

    void HostFile::close() {
        if(refCount_ > 0) return;
        int rc = ::close(hostFd_);
        verify(rc == 0);
    }

    bool HostFile::canRead() const {
        verify(false, "HostFile::canRead not implemented");
        return false;
    }

    bool HostFile::canWrite() const {
        verify(false, "HostFile::canWrite not implemented");
        return false;
    }

    ErrnoOrBuffer HostFile::read(size_t count, off_t offset) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        if(offset < 0) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::pread(hostFd_, buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t HostFile::write(const u8*, size_t, off_t) {
        // Host-backed files must be read-only
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

    ErrnoOrBuffer HostFile::getdents64(size_t count) {
        std::vector<u8> buf;
        buf.resize(count, 0x0);
        ssize_t nbytes = ::getdents64(hostFd_, buf.data(), buf.size());
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buf.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }

    int HostFile::fcntl(int cmd, int arg) {
        switch(cmd) {
            case F_GETFD:
            case F_SETFD:
            case F_GETFL:
            case F_SETFL: {
                int ret = ::fcntl(hostFd_, cmd, arg);
                if(ret < 0) return -errno;
                return ret;
            }
        }
        warn([&](){ fmt::print(fg(fmt::color::red), "implement missing fcntl {} on HostFile\n", cmd); });
        return -ENOTSUP;
    }

    ErrnoOrBuffer HostFile::ioctl(unsigned long request, const Buffer& inputBuffer) {
        switch(request) {
            case TCGETS: {
                struct termios ts;
                int ret = ::ioctl(hostFd_, TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                std::vector<u8> buffer;
                buffer.resize(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(Buffer{std::move(buffer)});
            }
            case FIOCLEX: {
                int ret = ::ioctl(hostFd_, FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case FIONCLEX: {
                int ret = ::ioctl(hostFd_, FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case TIOCGWINSZ: {
                struct winsize ws;
                int ret = ::ioctl(hostFd_, TIOCGWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                std::vector<u8> buffer;
                buffer.resize(sizeof(ws), 0x0);
                std::memcpy(buffer.data(), &ws, sizeof(ws));
                return ErrnoOrBuffer(Buffer{std::move(buffer)});
            }
            case TIOCSWINSZ: {
                struct winsize ws;
                std::memcpy(&ws, inputBuffer.data(), sizeof(ws));
                int ret = ::ioctl(hostFd_, TIOCSWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case TCSETSW: {
                struct termios ts;
                std::memcpy(&ts, inputBuffer.data(), sizeof(ts));
                int ret = ::ioctl(hostFd_, TCSETSW, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
        }
        verify(false, [&]() {
            fmt::print("implement ioctl {:#x} on HostFile\n", request);
        });
        return ErrnoOrBuffer(-ENOTSUP);
    }

}