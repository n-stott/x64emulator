#ifndef FSOBJET_H
#define FSOBJET_H

#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include "utils.h"
#include <cassert>
#include <optional>
#include <sys/types.h>

namespace kernel {

    class FS;

    class FsObject {
    public:
        explicit FsObject(FS* fs) : fs_(fs) { }
        virtual ~FsObject() = default;

        virtual void close() = 0;
        virtual bool keepAfterClose() const = 0;

        u32 refCount() const { return refCount_; }
        void ref() { ++refCount_; }
        void unref() { assert(refCount_ > 0); --refCount_; }

        virtual bool isFile() const { return false; }
        virtual bool isEpoll() const { return false; }
        virtual bool isSocket() const { return false; }

        virtual bool isPollable() const { return false; }

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual ErrnoOrBuffer read(size_t count) = 0;
        virtual ssize_t write(const u8* buf, size_t count) = 0;

        virtual std::optional<int> hostFileDescriptor() const = 0;

    protected:
        FS* fs_;
        u32 refCount_ { 0 };

    };

}

#endif