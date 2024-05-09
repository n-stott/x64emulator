#ifndef SHADOWFILE_H
#define SHADOWFILE_H

#include "fs/file.h"
#include "fs/fs.h"
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    class ShadowFile : public File {
    public:
        static std::unique_ptr<ShadowFile> tryCreate(FS* fs, const std::string& path, bool create);

        bool isReadable() const { return true; }
        bool isWritable() const { return true; }

        void close() override;
        bool keepAfterClose() const override { return true; }

        ErrnoOrBuffer read(size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;
        
        ErrnoOrBuffer pread(size_t count, off_t offset) override;
        ssize_t pwrite(const u8* buf, size_t count, off_t offset) override;

        ErrnoOrBuffer stat() override;
        off_t lseek(off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        void truncate();
        void append();
        void setWritable(bool writable) { writable_ = writable; }

    private:
        ShadowFile(FS* fs, std::vector<u8> data) : File(fs), data_(data) { }
        std::vector<u8> data_;
        size_t offset_ { 0 };
        bool writable_ { false };
    };

}

#endif