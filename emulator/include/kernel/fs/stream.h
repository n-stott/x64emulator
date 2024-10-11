#ifndef STREAM_H
#define STREAM_H

#include "kernel/fs/file.h"

namespace kernel {

    class Stream : public File {
    public:
        enum class TYPE {
            IN = 0,
            OUT = 1,
            ERR = 2,
        };

        Stream(FS* fs, TYPE type) : File(fs), type_(type) { }

        bool isReadable() const override;
        bool isWritable() const override;

        void close() override;
        bool keepAfterClose() const override { return false; }

        std::optional<int> hostFileDescriptor() const override { return (int)type_; }

        ErrnoOrBuffer read(size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;

        ErrnoOrBuffer pread(size_t count, off_t offset) override;
        ssize_t pwrite(const u8* buf, size_t count, off_t offset) override;

        ErrnoOrBuffer stat() override;
        off_t lseek(off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

    private:
        TYPE type_;
    };

}

#endif