#ifndef FILE_H
#define FILE_H

#include "utils/buffer.h"
#include "utils/erroror.h"
#include "utils/utils.h"
#include <sys/types.h>

namespace kernel {

    class FS;

    class File {
    public:
        explicit File(FS* fs) : fs_(fs) { }
        virtual ~File() = default;

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual void close() = 0;
        virtual bool keepAfterClose() const = 0;

        virtual ErrnoOrBuffer read(size_t count) = 0;
        virtual ssize_t write(const u8* buf, size_t count) = 0;

        virtual ErrnoOrBuffer pread(size_t count, size_t offset) = 0;
        virtual ssize_t pwrite(const u8* buf, size_t count, size_t offset) = 0;

        virtual ErrnoOrBuffer stat() = 0;

    protected:
        FS* fs_;

    };

}

#endif