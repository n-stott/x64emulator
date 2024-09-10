#ifndef EPOLL_H
#define EPOLL_H

#include "kernel/fs/fsobject.h"

namespace kernel {

    class Epoll : public FsObject {
    public:
        explicit Epoll(FS* fs, int flags) : FsObject(fs), flags_(flags) { }

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const { return false; }
        bool isWritable() const { return false; }

        ErrnoOrBuffer read(size_t);
        ssize_t write(const u8*, size_t);

        std::optional<int> hostFileDescriptor() const override { return {}; }


    private:
        int flags_;
    };

}

#endif