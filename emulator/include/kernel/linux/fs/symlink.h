#ifndef SYMLINK_H
#define SYMLINK_H

#include "kernel/linux/fs/file.h"

namespace kernel::gnulinux {

    class Symlink : public File {
    public:
        bool isSymlink() const override { return true; }

        virtual ErrnoOrBuffer readlink(size_t bufferSize);
        const std::string& link() const { return link_; }

        bool isReadable() const override;
        bool isWritable() const override;
        bool canRead() const override;
        bool canWrite() const override;
        ReadResult read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;
        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int) override;
        ErrnoOrBuffer getdents64(size_t count) override;
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;
        std::string className() const override;

    protected:
        Symlink(std::string name, std::string link) : File(std::move(name)), link_(std::move(link)) { }

    private:
        std::string link_;
    };

}

#endif