#ifndef STREAM_H
#define STREAM_H

#include "fs/file.h"

namespace kernel {

    class Stream : public File {
    public:
        enum class TYPE {
            IN = 0,
            OUT = 1,
            ERR = 2,
        };

        Stream(FS* fs, TYPE type) : File(fs), type_(type) { }

        bool isReadable() const override;
        bool isWritable() const override;

        ssize_t read(u8* buf, size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;

    private:
        TYPE type_;
    };

}

#endif