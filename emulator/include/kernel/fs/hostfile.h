#ifndef HOSTFILE_H
#define HOSTFILE_H

#include "kernel/fs/regularfile.h"
#include "kernel/fs/fs.h"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace kernel {

    class HostFile : public RegularFile {
    public:
        static std::unique_ptr<HostFile> tryCreate(FS* fs, const std::string& path);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        ErrnoOrBuffer read(size_t count, off_t offset) override;
        ssize_t write(const u8* buf, size_t count, off_t offset) override;

        ErrnoOrBuffer stat() override;
        off_t lseek(off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("HostFile(realfd={})", hostFd_);
        }

    private:
        HostFile(FS* fs, std::string path, int hostFd) : RegularFile(fs), path_(std::move(path)), hostFd_(hostFd) { }
        std::string path_;
        int hostFd_ { -1 };
    };

}

#endif