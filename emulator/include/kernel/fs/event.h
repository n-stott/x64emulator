#ifndef EVENT_H
#define EVENT_H

#include "kernel/fs/file.h"
#include <fmt/core.h>
#include <memory>

namespace kernel {

    class Event : public File {
    public:
        static std::unique_ptr<Event> tryCreate(FS* fs, unsigned int initval, int flags);

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return false; }
        bool isWritable() const override { return false; }

        bool isPollable() const override { return true; }

        ErrnoOrBuffer read(size_t, off_t) override;
        ssize_t write(const u8*, size_t, off_t) override;

        off_t lseek(off_t offset, int whence) override;

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        std::string className() const override {
            return fmt::format("Event(realfd={})", hostFd_);
        }

    private:
        explicit Event(FS* fs, unsigned int initval, int flags, int hostFd);

        u64 counter_;
        int flags_;
        int hostFd_ { -1 };
    };

}

#endif