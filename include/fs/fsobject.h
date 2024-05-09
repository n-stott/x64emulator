#ifndef FSOBJET_H
#define FSOBJET_H

#include "utils/utils.h"
#include <cassert>

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

    protected:
        FS* fs_;
        u32 refCount_ { 0 };

    };

}

#endif