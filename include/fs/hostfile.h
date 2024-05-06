#ifndef HOSTFILE_H
#define HOSTFILE_H

#include "fs/file.h"
#include "fs/fs.h"
#include <memory>
#include <string>

namespace kernel {

    class HostFile : public File {
    public:
        static std::unique_ptr<HostFile> tryCreate(FS* fs, const std::string& path);

        bool isReadable() const { return true; }
        bool isWritable() const { return false; }
        
        void close() override;
        bool keepAfterClose() const override { return false; }

        ErrnoOrBuffer read(size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;

        ErrnoOrBuffer pread(size_t count, size_t offset) override;
        ssize_t pwrite(const u8* buf, size_t count, size_t offset) override;

        ErrnoOrBuffer stat() override;

    private:
        HostFile(FS* fs, std::string path, int hostFd) : File(fs), path_(std::move(path)), hostFd_(hostFd) { }
        std::string path_;
        int hostFd_ { -1 };
    };

}

#endif