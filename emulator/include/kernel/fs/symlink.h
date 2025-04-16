#ifndef SYMLINK_H
#define SYMLINK_H

#include "kernel/fs/file.h"

namespace kernel {

    class Symlink : public File {
    public:
        bool isSymlink() const override { return true; }

        virtual ErrnoOrBuffer readlink(size_t bufferSize);
        const std::string& link() const { return link_; }

        bool isReadable() const;
        bool isWritable() const;
        bool canRead() const;
        bool canWrite() const;
        ErrnoOrBuffer read(OpenFileDescription&, size_t count);
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count);
        off_t lseek(OpenFileDescription&, off_t offset, int whence);
        ErrnoOrBuffer stat();
        ErrnoOrBuffer statfs();
        ErrnoOrBuffer getdents64(size_t count);
        std::optional<int> fcntl(int cmd, int arg);
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer);
        std::string className() const;

    protected:
        Symlink(FS* fs, Directory* dir, std::string name, std::string link) : File(fs, dir, std::move(name)), link_(std::move(link)) { }

    private:
        std::string link_;
    };

}

#endif