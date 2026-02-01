#ifndef EVENT_H
#define EVENT_H

#include "kernel/linux/fs/file.h"
#include <memory>

namespace kernel::gnulinux {

    class Event : public File {
    public:
        static std::unique_ptr<Event> tryCreate(unsigned int initval, int flags);

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;

        ReadResult read(OpenFileDescription&, size_t) override;
        ssize_t write(OpenFileDescription&, const u8*, size_t) override;

        void advanceInternalOffset(off_t) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;

        std::optional<int> hostFileDescriptor() const override { return {}; }

        std::string className() const override {
            return "Event";
        }

    private:
        explicit Event(unsigned int initval, int flags);

        u64 counter_;
        int flags_;
    };

}

#endif