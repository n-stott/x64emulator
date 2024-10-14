#ifndef OPENFILEDESCRIPTION_H
#define OPENFILEDESCRIPTION_H

#include "kernel/fs/file.h"

namespace kernel {

    class File;

    class OpenFileDescription {
    public:
        struct Flags {

        };

        explicit OpenFileDescription(File* file, Flags flags) : file_(file), flags_(flags) { }

        File* file() { return file_; }

        ErrnoOrBuffer read(size_t count) {
            ErrnoOrBuffer errnoOrBuffer = file_->read(count, offset_);
            errnoOrBuffer.errorOrWith<int>([&](const Buffer& buf) {
                offset_ += buf.size();
                return 0;
            });
            return errnoOrBuffer;
        }

        ssize_t write(const u8* buf, size_t count) {
            ssize_t nbytes = file_->write(buf, count, offset_);
            if(nbytes < 0) return nbytes;
            offset_ += nbytes;
            return nbytes;
        }

        ErrnoOrBuffer pread(size_t count, off_t offset) {
            return file_->read(count, offset);
        }

        ssize_t pwrite(const u8* buf, size_t count, off_t offset) {
            return file_->write(buf, count, offset);
        }

    private:
        File* file_ { nullptr };
        off_t offset_ { 0 };
        Flags flags_;
    };

}

#endif