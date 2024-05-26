#ifndef EVENT_H
#define EVENT_H

#include "fs/fsobject.h"
#include <memory>

namespace kernel {

    class Event : public FsObject {
    public:
        static std::unique_ptr<Event> tryCreate(FS* fs, unsigned int initval, int flags);

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const { return false; }
        bool isWritable() const { return false; }

        bool isPollable() const { return true; }

        ErrnoOrBuffer read(size_t);
        ssize_t write(const u8*, size_t);

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

    private:
        explicit Event(FS* fs, unsigned int initval, int flags, int hostFd);

        unsigned int initval_;
        int flags_;
        int hostFd_ { -1 };
    };

}

#endif