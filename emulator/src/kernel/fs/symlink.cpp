#include "kernel/fs/symlink.h"
#include "verify.h"

namespace kernel {

    ErrnoOrBuffer Symlink::readlink(size_t bufferSize) {
        if(bufferSize == 0) return ErrnoOrBuffer(-EINVAL);
        std::vector<char> linkpath(link_.begin(), link_.end());
        if(linkpath.size() > bufferSize) linkpath.resize(bufferSize);
        return ErrnoOrBuffer(Buffer(linkpath));
    }

    bool Symlink::isReadable() const {
        verify(false, "Symlink::isReadable not implemented");
        return true;
    }

    bool Symlink::isWritable() const {
        verify(false, "Symlink::isWritable not implemented");
        return true;
    }

    bool Symlink::canRead() const {
        verify(false, "Symlink::canRead not implemented");
        return true;
    }

    bool Symlink::canWrite() const {
        verify(false, "Symlink::canWrite not implemented");
        return true;
    }

    ErrnoOrBuffer Symlink::read(OpenFileDescription&, size_t) {
        verify(false, "Symlink::read not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ssize_t Symlink::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "Symlink::write not implemented");
        return -ENOTSUP;
    }

    off_t Symlink::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "Symlink::lseek not implemented");
        return -ENOTSUP;
    }

    ErrnoOrBuffer Symlink::stat() {
        verify(false, "Symlink::stat not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Symlink::statfs() {
        verify(false, "Symlink::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Symlink::getdents64(size_t) {
        verify(false, "Symlink::getdents64 not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    std::optional<int> Symlink::fcntl(int, int) {
        verify(false, "Symlink::fcntl not implemented");
        return {};
    }

    ErrnoOrBuffer Symlink::ioctl(OpenFileDescription&, Ioctl, const Buffer&) {
        verify(false, "Symlink::ioctl not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    std::string Symlink::className() const {
        return "Symlink";
    }

}