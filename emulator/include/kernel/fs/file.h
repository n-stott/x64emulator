#ifndef FILE_H
#define FILE_H

#include "kernel/fs/fsobject.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <sys/types.h>

namespace kernel {

    class File : public FsObject {
    public:
        explicit File(FS* fs) : FsObject(fs) { }

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual ErrnoOrBuffer read(size_t count) = 0;
        virtual ssize_t write(const u8* buf, size_t count) = 0;
    };

}

#endif