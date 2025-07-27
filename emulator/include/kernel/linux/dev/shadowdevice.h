#ifndef SHADOWDEVICE_H
#define SHADOWDEVICE_H

#include "kernel/linux/dev/device.h"
#include "kernel/linux/fs/fs.h"
#include <fmt/core.h>
#include <memory>
#include <optional>
#include <string>

namespace kernel {

    class Directory;

    class ShadowDevice : public Device {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname);

        static std::optional<int> tryGetDeviceHostFd(const std::string& pathname);

        bool isShadow() const override { return true; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return true; }

        bool isPollable() const override { return false; }
        bool canRead() const override;
        bool canWrite() const override;
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("ShadowDevice(realfd={})", hostFd_.value_or(-1));
        }

    protected:
        ShadowDevice(FS* fs, Directory* dir, std::string name, std::optional<int> hostFd) : Device(fs, dir, std::move(name)), hostFd_(hostFd) { }
        std::optional<int> hostFd_ { -1 };

        static std::vector<std::string> allAllowedDevices_;
    };

}

#endif