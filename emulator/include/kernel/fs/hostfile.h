#ifndef HOSTFILE_H
#define HOSTFILE_H

#include "kernel/fs/regularfile.h"
#include "kernel/fs/fs.h"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace kernel {

    class Directory;

    class HostFile : public RegularFile {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname, BitFlags<FS::AccessMode> accessMode, bool closeOnExec);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("HostFile(realfd={})", hostFd_);
        }

    private:
        HostFile(FS* fs, Directory* dir, std::string name, int hostFd) : RegularFile(fs, dir, std::move(name)), hostFd_(hostFd) { }
        int hostFd_ { -1 };
    };

}

#endif