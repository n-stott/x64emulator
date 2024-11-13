#ifndef TTYDEVICE_H
#define TTYDEVICE_H

#include "kernel/fs/shadowdevice.h"
#include "kernel/fs/fs.h"
#include <fmt/core.h>
#include <memory>
#include <optional>
#include <string>

namespace kernel {

    class Directory;

    class TtyDevice : public ShadowDevice {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        ErrnoOrBuffer read(size_t count, off_t offset) override;
        ssize_t write(const u8* buf, size_t count, off_t offset) override;

        off_t lseek(off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("TtyDevice(realfd={})", hostFd_.value_or(-1));
        }

    private:
        TtyDevice(FS* fs, Directory* dir, std::string name, std::optional<int> hostFd) : ShadowDevice(fs, dir, std::move(name), hostFd) { }
    };

}

#endif