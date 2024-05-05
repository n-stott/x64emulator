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

        ssize_t read(u8* buf, size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;

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