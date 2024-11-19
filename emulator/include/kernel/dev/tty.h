#ifndef TTYDEVICE_H
#define TTYDEVICE_H

#include "kernel/dev/shadowdevice.h"
#include "kernel/fs/fs.h"
#include <fmt/core.h>
#include <memory>
#include <optional>
#include <string>

namespace kernel {

    class Directory;

    class Tty : public ShadowDevice {
    public:
        static Tty* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override { return true; }
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        ErrnoOrBuffer stat() override;

        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("Tty(realfd={})", hostFd_.value_or(-1));
        }

    private:
        Tty(FS* fs, Directory* dir, std::string name, std::optional<int> hostFd) : ShadowDevice(fs, dir, std::move(name), hostFd) { }
    };

}

#endif