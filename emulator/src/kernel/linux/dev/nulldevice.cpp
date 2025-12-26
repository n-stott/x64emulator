#include "kernel/linux/dev/nulldevice.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"

namespace kernel::gnulinux {

    std::unique_ptr<NullDevice> NullDevice::tryCreate(const Path& path) {
        return std::unique_ptr<NullDevice>(new NullDevice(path.last()));
    }

    void NullDevice::close() { }

    ErrnoOrBuffer NullDevice::read(OpenFileDescription&, size_t) {
        return ErrnoOrBuffer(Buffer{});
    }

    ssize_t NullDevice::write(OpenFileDescription&, const u8*, size_t count) {
        return (ssize_t)count;
    }

    ErrnoOrBuffer NullDevice::stat() {
        return Host::stat(path().absolute());
    }

    ErrnoOrBuffer NullDevice::statfs() {
        verify(false, "NullDevice::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer NullDevice::statx(unsigned int mask) {
        verify(false, fmt::format("File::statx(mask={:#x}) not implemented for file type {}\n", mask, className()));
        return ErrnoOrBuffer(-ENOTSUP);
    }

    off_t NullDevice::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "NullDevice::lseek not implemented");
        return -ENOTSUP;
    }

    std::optional<int> NullDevice::fcntl(int, int) {
        verify(false, "NullDevice::fcntl not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer NullDevice::ioctl(OpenFileDescription&, Ioctl, const Buffer&) {
        return ErrnoOrBuffer(-ENOTTY);
    }
}