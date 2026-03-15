#ifndef HOSTDEVICE_H
#define HOSTDEVICE_H

#include "kernel/linux/dev/device.h"
#include "host/host.h"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace kernel::gnulinux {

    class Directory;
    class Path;

    class HostDevice : public Device {
    public:
        static std::unique_ptr<HostDevice> tryCreate(const Path& path);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return handle_->fd().fd; }

        ReadResult read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        void advanceInternalOffset(off_t) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;

        std::string className() const override {
            return fmt::format("HostDevice(realfd={})", handle_->fd().fd);
        }

    private:
        HostDevice(std::string name, std::optional<Host::FileHandle> handle) :
                Device(std::move(name)),
                handle_(std::move(handle)) { }
        std::optional<Host::FileHandle> handle_;
    };

}

#endif