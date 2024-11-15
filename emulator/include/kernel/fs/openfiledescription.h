#ifndef OPENFILEDESCRIPTION_H
#define OPENFILEDESCRIPTION_H

#include "kernel/fs/file.h"
#include "bitflags.h"
#include "verify.h"

namespace kernel {

    class File;

    class OpenFileDescription {
    public:
        // keep aligned with FS::StatusFlags
        enum class StatusFlags {
            APPEND    = (1 << 0),
            ASYNC     = (1 << 1),
            DIRECT    = (1 << 2),
            DSYNC     = (1 << 3),
            LARGEFILE = (1 << 4),
            NDELAY    = (1 << 5),
            NOATIME   = (1 << 6),
            NONBLOCK  = (1 << 7),
            PATH      = (1 << 8),
            RDWR      = (1 << 9),
            SYNC      = (1 << 10),
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

        explicit OpenFileDescription(File* file, BitFlags<StatusFlags> flags) : file_(file), flags_(flags) {
            (void)flags_;
        }

        File* file() { return file_; }
        BitFlags<StatusFlags>& flags() { return flags_; }
        off_t offset() const { return offset_; }
        
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
            ErrnoOrBuffer errnoOrBuffer = file_->read(*this, count);
            errnoOrBuffer.errorOrWith<int>([&](const Buffer& buf) {
                offset_ += buf.size();
                return 0;
            });
            return errnoOrBuffer;
        }

        ssize_t write(const u8* buf, size_t count) {
            ssize_t nbytes = file_->write(*this, buf, count);
            if(nbytes < 0) return nbytes;
            offset_ += nbytes;
            return nbytes;
        }

        ErrnoOrBuffer pread(size_t count, off_t offset) {
            off_t savedOffset = offset_;
            offset_ = offset;
            auto result = file_->read(*this, count);
            offset_ = savedOffset;
            return result;
        }

        ssize_t pwrite(const u8* buf, size_t count, off_t offset) {
            off_t savedOffset = offset_;
            offset_ = offset;
            auto result = file_->write(*this, buf, count);
            offset_ = savedOffset;
            return result;
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
        BitFlags<StatusFlags> flags_;
        Lock lock_ { Lock::NONE };
    };

}

#endif