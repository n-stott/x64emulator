#ifndef FILE_H
#define FILE_H

#include "fs/fsobject.h"
#include "utils/buffer.h"
#include "utils/erroror.h"
#include "utils/utils.h"
#include <cassert>
#include <sys/types.h>

namespace kernel {

    class File : public FsObject {
    public:
        explicit File(FS* fs) : FsObject(fs) { }

        bool isFile() const override { return true; }

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual ErrnoOrBuffer read(size_t count) = 0;
        virtual ssize_t write(const u8* buf, size_t count) = 0;

        virtual ErrnoOrBuffer pread(size_t count, off_t offset) = 0;
        virtual ssize_t pwrite(const u8* buf, size_t count, off_t offset) = 0;

        virtual ErrnoOrBuffer stat() = 0;
        virtual off_t lseek(off_t offset, int whence) = 0;
        
        virtual ErrnoOrBuffer getdents64(size_t count) = 0;
        virtual int fcntl(int cmd, int arg) = 0;
        virtual ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) = 0;
    };

}

#endif