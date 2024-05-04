#ifndef FILE_H
#define FILE_H

#include "utils/buffer.h"
#include "utils/erroror.h"
#include "utils/utils.h"

namespace kernel {

    class FS;

    class File {
    public:
        explicit File(FS* fs) : fs_(fs) { }
        virtual ~File() = default;

        virtual ssize_t read(u8* buf, size_t count) const = 0;
        virtual ssize_t write(const u8* buf, size_t count) = 0;

    protected:
        FS* fs_;

    };

}

#endif