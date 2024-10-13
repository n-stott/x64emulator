#ifndef REGULARFILE_H
#define REGULARFILE_H

#include "kernel/fs/file.h"
#include "utils.h"
#include <cassert>
#include <sys/types.h>

namespace kernel {

    class RegularFile : public File {
    public:
        explicit RegularFile(FS* fs) : File(fs) { }

        bool isRegularFile() const override { return true; }

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