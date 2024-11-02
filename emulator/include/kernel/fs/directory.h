#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "kernel/fs/file.h"
#include "utils.h"
#include <cassert>
#include <memory>
#include <sys/types.h>

namespace kernel {

    class Directory : public File {
    public:
        explicit Directory(FS* fs, Directory* parent, std::string name) : File(fs, parent, std::move(name)) { }

        bool isDirectory() const override final { return true; }
        void printSubtree() const;

        Directory* tryGetOrAddSubDirectory(std::string name);

        void close() override;
        bool keepAfterClose() const override { return true; }

        std::optional<int> hostFileDescriptor() const override { return {}; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        bool canRead() const override { return false; }
        bool canWrite() const override { return false; }

        ErrnoOrBuffer read(size_t count, off_t offset) override;
        ssize_t write(const u8* buf, size_t count, off_t offset) override;

        off_t lseek(off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override { return "directory"; }

    private:
        std::vector<std::unique_ptr<File>> entries_;
    };

}

#endif