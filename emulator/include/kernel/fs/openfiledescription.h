#ifndef OPENFILEDESCRIPTION_H
#define OPENFILEDESCRIPTION_H

#include "kernel/fs/file.h"
#include "verify.h"

namespace kernel {

    class File;

    class OpenFileDescription {
    public:
        struct Flags {

        };

        enum class Lock {
            NONE,
            SHARED,
            EXCLUSIVE,
        };

        enum class Blocking {
            NO,
            YES,
        };

        explicit OpenFileDescription(File* file, Flags flags) : file_(file), flags_(flags) { }

        File* file() { return file_; }
        
        bool isLockedExclusively() const { return lock_ == Lock::EXCLUSIVE; }
        bool isLockedShared() const { return lock_ == Lock::SHARED; }
        
        [[nodiscard]] int tryLock(Lock lock, Blocking blocking) {
            if(blocking == Blocking::NO && lock_ == Lock::EXCLUSIVE) return -EWOULDBLOCK;
            verify(lock_ == Lock::NONE, "Lock contention not supported");
            lock_ = lock;
            return 0;
        }

        void unlock() {
            lock_ = Lock::NONE;
        }



        ErrnoOrBuffer read(size_t count) {
            ErrnoOrBuffer errnoOrBuffer = file_->read(count, offset_);
            errnoOrBuffer.errorOrWith<int>([&](const Buffer& buf) {
                offset_ += buf.size();
                return 0;
            });
            return errnoOrBuffer;
        }

        ssize_t write(const u8* buf, size_t count) {
            ssize_t nbytes = file_->write(buf, count, offset_);
            if(nbytes < 0) return nbytes;
            offset_ += nbytes;
            return nbytes;
        }

        ErrnoOrBuffer pread(size_t count, off_t offset) {
            return file_->read(count, offset);
        }

        ssize_t pwrite(const u8* buf, size_t count, off_t offset) {
            return file_->write(buf, count, offset);
        }

        off_t lseek(off_t offset, int whence) {
            off_t newoffset = file_->lseek(offset, whence);
            if(newoffset < 0) return newoffset;
            offset_ = newoffset;
            return newoffset;
        }

        std::string toString() const {
            return file_->className();
        }

    private:
        File* file_ { nullptr };
        off_t offset_ { 0 };
        Flags flags_;
        Lock lock_ { Lock::NONE };
    };

}

#endif