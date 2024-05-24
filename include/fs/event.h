#ifndef EVENT_H
#define EVENT_H

#include "fs/fsobject.h"

namespace kernel {

    class Event : public FsObject {
    public:
        explicit Event(FS* fs, unsigned int initval, int flags) : FsObject(fs), initval_(initval), flags_(flags) { }

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const { return false; }
        bool isWritable() const { return false; }

        ErrnoOrBuffer read(size_t);
        ssize_t write(const u8*, size_t);

        std::optional<int> hostFileDescriptor() const override { return {}; }


    private:
        unsigned int initval_;
        int flags_;
    };

}

#endif