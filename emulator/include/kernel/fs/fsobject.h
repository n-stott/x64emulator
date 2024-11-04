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
        
        bool deleteAfterClose() const { return deleteAfterClose_; }
        void setDeleteAfterClose() { deleteAfterClose_ = true; }

        u32 refCount() const { return refCount_; }
        void ref() { ++refCount_; }
        void unref() { assert(refCount_ > 0); --refCount_; }

        virtual bool isRegularFile() const { return false; }
        virtual bool isDirectory() const { return false; }
        virtual bool isEpoll() const { return false; }
        virtual bool isSocket() const { return false; }
        virtual bool isPipe() const { return false; }

        virtual bool isPollable() const { return false; }

        virtual std::optional<int> hostFileDescriptor() const = 0;

    protected:
        FS* fs_;
        u32 refCount_ { 0 };
        bool deleteAfterClose_ { false };

    };

}

#endif