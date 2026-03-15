#include "kernel/linux/dev/hostdevice.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel::gnulinux {

    std::unique_ptr<HostDevice> HostDevice::tryCreate(const Path& path) {
        std::string pathname = path.absolute();
        auto handle = Host::tryOpen(pathname.c_str(), Host::FileType::DEVICE, Host::CloseOnExec::YES);
        if(!handle) return {};
        return std::unique_ptr<HostDevice>(new HostDevice(path.last(), std::move(handle)));
    }

    void HostDevice::close() {
        if(refCount_ > 0) return;
        handle_.reset();
    }

    bool HostDevice::canRead() const {
        return Host::pollCanRead(handle_->fd());
    }

    bool HostDevice::canWrite() const {
        verify(false, "HostDevice::canWrite not implemented");
        return false;
    }

    ReadResult HostDevice::read(OpenFileDescription&, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        Buffer buffer(count, 0x0);
        ssize_t nbytes = handle_->read(buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ssize_t HostDevice::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "HostDevice::write not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer HostDevice::stat() {
        return Host::stat(path().absolute());
    }

    ErrnoOrBuffer HostDevice::statfs() {
        verify(false, "HostDevice::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer HostDevice::statx(unsigned int mask) {
        verify(false, fmt::format("File::statx(mask={:#x}) not implemented for file type {}\n", mask, className()));
        return ErrnoOrBuffer(-ENOTSUP);
    }

    void HostDevice::advanceInternalOffset(off_t offset) {
        off_t ret = handle_->lseek(offset, Host::FileHandle::SEEK::CUR);
        verify(ret >= 0, "advanceInternalOffset failed in HostDevice");
    }

    off_t HostDevice::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "HostDevice::lseek not implemented");
        return -ENOTSUP;
    }

    std::optional<int> HostDevice::fcntl(int cmd, int arg) {
        return Host::fcntl(handle_->fd(), cmd, arg);
    }

    ErrnoOrBuffer HostDevice::ioctl(OpenFileDescription&, Ioctl, const Buffer&) {
        verify(false, "HostDevice::ioctl not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

}