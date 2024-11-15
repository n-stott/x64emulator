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

        File* tryGetEntry(const std::string& name);
        std::unique_ptr<File> tryTakeEntry(const std::string& name);
        Directory* tryGetSubDirectory(const std::string& name);

        Directory* tryAddHostDirectory(const std::string& name);
        Directory* tryAddShadowDirectory(const std::string& name);

        bool hasBeenTaintedByShadow() const { return taintedByShadow_; }
        void setTaintedByShadow() { taintedByShadow_ = true; }

        template<typename FileType>
        FileType* addFile(std::unique_ptr<FileType> file) {
            FileType* ptr = file.get();
            entries_.push_back(std::move(file));
            if(ptr->isShadow()) setTaintedByShadow();
            return ptr;
        }

        void close() override;
        bool keepAfterClose() const override { return true; }

        std::optional<int> hostFileDescriptor() const override { return {}; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        bool canRead() const override { return false; }
        bool canWrite() const override { return false; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        off_t lseek(off_t offset, int whence) override;

        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override { return "directory"; }

    private:
        std::vector<std::unique_ptr<File>> entries_;
        bool taintedByShadow_ { false };
    };

}

#endif