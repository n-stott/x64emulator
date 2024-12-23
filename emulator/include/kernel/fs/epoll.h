#ifndef EPOLL_H
#define EPOLL_H

#include "kernel/fs/file.h"

namespace kernel {

    class Epoll : public File {
    public:
        explicit Epoll(FS* fs, int flags) : File(fs), flags_(flags) { }

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        // non pollable
        bool canRead() const override { return false; }
        bool canWrite() const override { return false; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t) override;
        ssize_t write(OpenFileDescription&, const u8*, size_t) override;

        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        std::optional<int> hostFileDescriptor() const override { return {}; }

        std::string className() const override {
            return "Epoll";
        }

        ErrnoOr<void> addEntry(i32 fd, u32 event, u64 data);
        ErrnoOr<void> changeEntry(i32 fd, u32 event, u64 data);
        ErrnoOr<void> deleteEntry(i32 fd);

    private:
        struct Entry {
            i32 fd;
            u32 event;
            u64 data;
        };

        std::vector<Entry> interestList_;
        int flags_;
    };

}

#endif