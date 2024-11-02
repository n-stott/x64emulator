#ifndef FILE_H
#define FILE_H

#include "kernel/fs/fsobject.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <sys/types.h>
#include <string>

namespace kernel {

    class Directory;

    class File : public FsObject {
    public:
        explicit File(FS* fs) : FsObject(fs), parent_(nullptr), name_("_anonymous_file_") { }
        explicit File(FS* fs, Directory* parent, std::string name) : FsObject(fs), parent_(parent), name_(std::move(name)) { }

        virtual std::string path() const;
        virtual std::string name() const { return name_; }

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual bool canRead() const = 0;
        virtual bool canWrite() const = 0;

        virtual ErrnoOrBuffer read(size_t count, off_t offset) = 0;
        virtual ssize_t write(const u8* buf, size_t count, off_t offset) = 0;

        virtual off_t lseek(off_t offset, int whence) = 0;

        virtual ErrnoOrBuffer stat() = 0;
        
        virtual ErrnoOrBuffer getdents64(size_t count) = 0;

        virtual int fcntl(int cmd, int arg) = 0;
        virtual ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) = 0;

        virtual std::string className() const = 0;

    private:
        Directory* parent_ { nullptr };
        std::string name_;
    };

}

#endif