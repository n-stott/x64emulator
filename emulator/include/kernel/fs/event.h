#ifndef EVENT_H
#define EVENT_H

#include "kernel/fs/file.h"
#include <fmt/core.h>
#include <memory>

namespace kernel {

    class Event : public File {
    public:
        static std::unique_ptr<Event> tryCreate(FS* fs, unsigned int initval, int flags);

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;

        ErrnoOrBuffer read(OpenFileDescription&, size_t) override;
        ssize_t write(OpenFileDescription&, const u8*, size_t) override;

        off_t lseek(off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        std::string className() const override {
            return fmt::format("Event(realfd={})", hostFd_);
        }

    private:
        explicit Event(FS* fs, unsigned int initval, int flags, int hostFd);

        u64 counter_;
        int flags_;
        int hostFd_ { -1 };
    };

}

#endif