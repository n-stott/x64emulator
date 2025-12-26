#ifndef OPENFILEDESCRIPTION_H
#define OPENFILEDESCRIPTION_H

#include "kernel/linux/fs/file.h"
#include "kernel/linux/fs/fsflags.h"
#include "bitflags.h"
#include "verify.h"

namespace kernel::gnulinux {

    class File;

    class OpenFileDescription {
    public:
        enum class Lock {
            NONE,
            SHARED,
            EXCLUSIVE,
        };

        enum class Blocking {
            NO,
            YES,
        };

        explicit OpenFileDescription(File* file, BitFlags<AccessMode> accessMode, BitFlags<StatusFlags> statusFlags) :
                file_(file), accessMode_(accessMode), statusFlags_(statusFlags) { }

        File* file() { return file_; }
        BitFlags<AccessMode>& accessMode() { return accessMode_; }
        BitFlags<StatusFlags>& statusFlags() { return statusFlags_; }
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
                file_->advanceInternalOffset((off_t)buf.size());
                return 0;
            });
            return errnoOrBuffer;
        }

        ssize_t write(const u8* buf, size_t count) {
            ssize_t nbytes = file_->write(*this, buf, count);
            if(nbytes < 0) return nbytes;
            offset_ += nbytes;
            file_->advanceInternalOffset(nbytes);
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
            off_t newoffset = file_->lseek(*this, offset, whence);
            if(newoffset < 0) return newoffset;
            offset_ = newoffset;
            return newoffset;
        }

        ErrnoOrBuffer ioctl(Ioctl request, const Buffer& buffer);

        std::string toString() const {
            return file_->className();
        }

    private:
        File* file_ { nullptr };
        off_t offset_ { 0 };
        BitFlags<AccessMode> accessMode_;
        BitFlags<StatusFlags> statusFlags_;
        Lock lock_ { Lock::NONE };
    };

}

#endif