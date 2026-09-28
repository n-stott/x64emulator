#ifndef HOSTFILE_H
#define HOSTFILE_H

#include "kernel/linux/fs/fsflags.h"
#include "kernel/linux/fs/regularfile.h"
#include "host/host.h"
#include "bitflags.h"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace kernel::gnulinux {

    class Directory;
    class Path;

    class HostFile : public RegularFile {
    public:
        static std::unique_ptr<HostFile> tryCreate(const Path& path, BitFlags<AccessMode> accessMode, bool closeOnExec);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        ReadResult read(OpenFileDescription&, size_t count) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t count) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;

        void advanceInternalOffset(off_t offset) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        std::optional<int> fcntl(FcntlCommand cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;

        std::string className() const override {
            return "HostFile";
        }

    private:
        explicit HostFile(std::string name) :
                RegularFile(std::move(name)) { }

        std::shared_ptr<Host::FileHandle> tryGetHandle() const;
    };

}

#endif