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
        verify(!accessMode.test(AccessMode::WRITE), "Hostfile is not writable");
        auto handle = Host::tryOpen(pathname.c_str(), Host::FileType::REGULAR_FILE,
                closeOnExec ? Host::CloseOnExec::YES : Host::CloseOnExec::NO);
        if(!handle) return {};
        return std::unique_ptr<HostFile>(new HostFile(path.last(), std::move(handle)));
    }

    void HostFile::close() {
        if(refCount_ > 0) return;
        handle_.reset();
    }

    bool HostFile::canRead() const {
        return Host::pollCanRead(handle_->fd());
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
        ssize_t nbytes = handle_->pread(buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ssize_t HostFile::write(OpenFileDescription&, const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

    ErrnoOrBuffer HostFile::stat() {
        return handle_->stat();
    }

    ErrnoOrBuffer HostFile::statfs() {
        return handle_->statfs();
    }

    ErrnoOrBuffer HostFile::statx(unsigned int mask) {
        return Host::statx(handle_->fd(), "", AT_EMPTY_PATH, mask);
    }

    void HostFile::advanceInternalOffset(off_t offset) {
        off_t ret = handle_->lseek(offset, Host::FileHandle::SEEK::CUR);
        verify(ret >= 0, []() {
            fmt::print("Expected no error in HostFile::advanceInternalOffset, but got errno = {}\n", errno);
        });
    }

    off_t HostFile::lseek(OpenFileDescription& ofd, off_t offset, int whence) {
        off_t ret = [&]() {
            if(whence == SEEK_CUR) {
                return handle_->lseek(ofd.offset() + offset, Host::FileHandle::SEEK::SET);
            } else if(whence == SEEK_SET) {
                return handle_->lseek(offset, Host::FileHandle::SEEK::SET);
            } else {
                verify(whence == SEEK_END);
                return handle_->lseek(offset, Host::FileHandle::SEEK::END);
            }
        }();
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer HostFile::getdents64(size_t count) {
        Buffer buf(count, 0x0);
        ssize_t nbytes = ::getdents64(handle_->fd().fd, buf.data(), buf.size());
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buf.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buf));
    }

    std::optional<int> HostFile::fcntl(FcntlCommand cmd, int arg) {
        int hostcmd = Host::Fcntl::fromCommand(cmd);
        return Host::fcntl(handle_->fd(), hostcmd, arg);
    }

    ErrnoOrBuffer HostFile::ioctl(OpenFileDescription&, Ioctl request, const Buffer& inputBuffer) {
        switch(request) {
            case Ioctl::tcgets: {
                struct termios ts;
                int ret = ::ioctl(handle_->fd().fd, TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                Buffer buffer(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::fioclex: {
                int ret = ::ioctl(handle_->fd().fd, FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::fionclex: {
                int ret = ::ioctl(handle_->fd().fd, FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::tiocgwinsz: {
                struct winsize ws;
                int ret = ::ioctl(handle_->fd().fd, TIOCGWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                Buffer buffer(sizeof(ws), 0x0);
                std::memcpy(buffer.data(), &ws, sizeof(ws));
                return ErrnoOrBuffer(std::move(buffer));
            }
            case Ioctl::tiocswinsz: {
                struct winsize ws;
                std::memcpy(&ws, inputBuffer.data(), sizeof(ws));
                int ret = ::ioctl(handle_->fd().fd, TIOCSWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case Ioctl::tcsetsw: {
                struct termios ts;
                std::memcpy(&ts, inputBuffer.data(), sizeof(ts));
                int ret = ::ioctl(handle_->fd().fd, TCSETSW, &ts);
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