#ifndef NULLDEVICE_H
#define NULLDEVICE_H

#include "kernel/linux/dev/device.h"
#include "kernel/linux/fs/fs.h"
#include <memory>
#include <optional>
#include <string>

namespace kernel::gnulinux {

    class Directory;

    class NullDevice : public Device {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname);

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return false; }
        bool canRead() const override { return false; }
        bool canWrite() const override { return false; }
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return {}; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;

        std::string className() const override {
            return "NullDevice";
        }

    protected:
        NullDevice(FS* fs, Directory* dir, std::string name) : Device(fs, dir, std::move(name)) { }
    };

}

#endif