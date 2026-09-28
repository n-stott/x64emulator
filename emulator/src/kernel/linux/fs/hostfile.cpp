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

    std::shared_ptr<Host::FileHandle> HostFile::tryGetHandle() const {
        auto absolutePath = path().absolute();
        auto handle = Host::tryOpen(absolutePath, Host::FileType::REGULAR_FILE, Host::Purgeable::YES, Host::CloseOnExec::YES);
        return handle;
    }

    std::unique_ptr<HostFile> HostFile::tryCreate(const Path& path, BitFlags<AccessMode> accessMode, bool closeOnExec) {
        std::string pathname = path.absolute();
        verify(!accessMode.test(AccessMode::WRITE), "Hostfile is not writable");
        auto handle = Host::tryOpen(pathname, Host::FileType::REGULAR_FILE,
                Host::Purgeable::YES,
                closeOnExec ? Host::CloseOnExec::YES : Host::CloseOnExec::NO);
        if(!handle) return {};
        return std::unique_ptr<HostFile>(new HostFile(path.last()));
    }

    void HostFile::close() {

    }

    bool HostFile::canRead() const {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return Host::pollCanRead(handle->fd());
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
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        ssize_t nbytes = handle->pread(buffer.data(), count, offset);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ssize_t HostFile::write(OpenFileDescription&, const u8*, size_t) {
        // Host-backed files must be read-only
        return -EINVAL;
    }

    ErrnoOrBuffer HostFile::stat() {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return handle->stat();
    }

    ErrnoOrBuffer HostFile::statfs() {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return handle->statfs();
    }

    ErrnoOrBuffer HostFile::statx(unsigned int mask) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return Host::statx(handle->fd(), "", AT_EMPTY_PATH, mask);
    }

    void HostFile::advanceInternalOffset(off_t offset) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        off_t ret = handle->lseek(offset, Host::FileHandle::SEEK::CUR);
        verify(ret >= 0, []() {
            fmt::print("Expected no error in HostFile::advanceInternalOffset, but got errno = {}\n", errno);
        });
    }

    off_t HostFile::lseek(OpenFileDescription& ofd, off_t offset, int whence) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        off_t ret = [&]() {
            if(whence == SEEK_CUR) {
                return handle->lseek(ofd.offset() + offset, Host::FileHandle::SEEK::SET);
            } else if(whence == SEEK_SET) {
                return handle->lseek(offset, Host::FileHandle::SEEK::SET);
            } else {
                verify(whence == SEEK_END);
                return handle->lseek(offset, Host::FileHandle::SEEK::END);
            }
        }();
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer HostFile::getdents64(size_t count) {
        Buffer buf(count, 0x0);
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        ssize_t nbytes = ::getdents64(handle->fd().fd, buf.data(), buf.size());
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buf.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buf));
    }

    std::optional<int> HostFile::fcntl(FcntlCommand cmd, int arg) {
        int hostcmd = Host::Fcntl::fromCommand(cmd);
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return Host::fcntl(handle->fd(), hostcmd, arg);
    }

    ErrnoOrBuffer HostFile::ioctl(OpenFileDescription&, Ioctl request, const Buffer& inputBuffer) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        auto res = handle->ioctl(request, inputBuffer);
        verify(res.errorOr(0) != -ENOTSUP, [&]() {
            fmt::print("implement ioctl {:#x} on HostFile\n", (int)request);
        });
        return res;
    }

}