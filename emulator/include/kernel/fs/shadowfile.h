#ifndef SHADOWFILE_H
#define SHADOWFILE_H

#include "kernel/fs/regularfile.h"
#include "kernel/fs/fs.h"
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    class ShadowFile : public RegularFile {
    public:
        static std::unique_ptr<ShadowFile> tryCreate(FS* fs, const std::string& path, bool create);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return true; }

        std::optional<int> hostFileDescriptor() const override { return {}; }
        
        ErrnoOrBuffer read(size_t count, off_t offset) override;
        ssize_t write(const u8* buf, size_t count, off_t offset) override;

        ErrnoOrBuffer stat() override;
        off_t lseek(off_t offset, int whence) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;
        int fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;

        void truncate();
        void append();
        void setWritable(bool writable) { writable_ = writable; }

        std::string className() const override {
            return "ShadowFile";
        }

    private:
        ShadowFile(FS* fs, std::vector<u8> data) : RegularFile(fs), data_(data) { }
        std::vector<u8> data_;
        bool writable_ { false };
    };

}

#endif