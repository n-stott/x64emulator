#ifndef INOTIFY_H
#define INOTIFY_H

#include "kernel/linux/fs/file.h"

namespace kernel::gnulinux {

    class Inotify : public File {
    public:
        explicit Inotify(int flags) : flags_(flags) {
            verify(flags == 0, "nonzero flags not supported in inotify");
        }

        void close() override {
            // nothing to do
        }

        bool keepAfterClose() const override {
            return false;
        }

        bool isPollable() const override {
            return true;
        }

        bool isReadable() const override {
            verify(false, "Inotify::isReadable not implemented");
            return false;
        }

        bool isWritable() const override {
            verify(false, "Inotify::isWritable not implemented");
            return false;
        }

        bool canRead() const override {
            return false;
        }

        bool canWrite() const override {
            verify(false, "Inotify::canWrite not implemented");
            return false;
        }

        ReadResult read(OpenFileDescription&, size_t) override {
            verify(false, "Inotify::read not implemented");
            return {};
        }

        ssize_t write(OpenFileDescription&, const u8*, size_t) override {
            verify(false, "Inotify::write not implemented");
            return -1;
        }

        off_t lseek(OpenFileDescription&, off_t, int) override {
            verify(false, "Inotify::lseek not implemented");
            return -1;
        }

        ErrnoOrBuffer stat() override {
            verify(false, "Inotify::stat not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }

        ErrnoOrBuffer statfs() override {
            verify(false, "Inotify::statfs not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }

        ErrnoOrBuffer statx(unsigned int) override {
            verify(false, "Inotify::statx not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }

        ErrnoOrBuffer getdents64(size_t) override {
            verify(false, "Inotify::getdents64 not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }

        std::optional<int> fcntl(FcntlCommand, int) override {
            warn("Inotify::fcntl not implemented");
            return {};
        }

        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl, const Buffer&) override {
            verify(false, "Inotify::ioctl not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }

        std::string className() const override {
            return "Inotify";
        }


    private:
        int flags_ { 0 };
    };

}

#endif