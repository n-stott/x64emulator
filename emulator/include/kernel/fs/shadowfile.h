#ifndef SHADOWFILE_H
#define SHADOWFILE_H

#include "kernel/fs/regularfile.h"
#include "kernel/fs/fs.h"
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    // pimpl
    struct ShadowFileHostData;

    class ShadowFile : public RegularFile {
    public:
        static ShadowFile* tryCreateAndAdd(FS* fs, Directory* dir, std::string name, bool create);
        ~ShadowFile();

        bool isShadow() const override { return true; }

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        // non pollable
        bool canRead() const override { return false; }
        bool canWrite() const override { return false; }

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
        ShadowFile(FS* fs, Directory* parent, std::string name, std::vector<u8> data);

        std::unique_ptr<ShadowFileHostData> hostData_;
        std::vector<u8> data_;
        bool writable_ { false };
    };

}

#endif