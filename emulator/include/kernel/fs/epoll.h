#ifndef EPOLL_H
#define EPOLL_H

#include "kernel/fs/file.h"

namespace kernel {

    class Epoll : public File {
    public:
        explicit Epoll(FS* fs, int flags) : File(fs), flags_(flags) { }

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        ErrnoOrBuffer read(size_t) override;
        ssize_t write(const u8*, size_t) override;

        std::optional<int> hostFileDescriptor() const override { return {}; }


    private:
        int flags_;
    };

}

#endif