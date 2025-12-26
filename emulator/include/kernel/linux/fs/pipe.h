#ifndef FIFO_H
#define FIFO_H

#include "kernel/linux/fs/file.h"
#include <deque>
#include <memory>
#include <vector>

namespace kernel::gnulinux {

    enum class PipeSide {
        READ,
        WRITE,
    };

    class PipeEndpoint;

    class Pipe : public FsObject {
    public:
        static std::unique_ptr<Pipe> tryCreate(int flags);

        std::unique_ptr<PipeEndpoint> tryCreateReader();
        std::unique_ptr<PipeEndpoint> tryCreateWriter();

        void closedEndpoint(const PipeEndpoint*);

        void close() override;
        bool isClosed() const { return isClosed_; }
        bool keepAfterClose() const override { return false; }
        std::optional<int> hostFileDescriptor() const override { return {}; };

        bool canRead() const;
        bool canWrite() const;

        ErrnoOrBuffer read(OpenFileDescription&, size_t size);
        ssize_t write(OpenFileDescription&, const u8* buf, size_t size);

    private:
        explicit Pipe(int flags) : flags_(flags) { }
        std::deque<u8> data_;
        int flags_;
        std::vector<PipeEndpoint*> readEndpoints_;
        std::vector<PipeEndpoint*> writeEndpoints_;
        bool isClosed_ { false };
    };

    class PipeEndpoint : public File {
    public:
        static std::unique_ptr<PipeEndpoint> tryCreate(Pipe* pipe, PipeSide side, int flags);

        bool isPipe() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isReadable() const override { return side_ == PipeSide::READ; }
        bool isWritable() const override { return side_ == PipeSide::WRITE; }

        bool isPollable() const override { return true; }

        bool canRead() const override;
        bool canWrite() const override;

        ErrnoOrBuffer read(OpenFileDescription&, size_t) override;
        ssize_t write(OpenFileDescription&, const u8*, size_t) override;

        void advanceInternalOffset(off_t) override;
        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int) override;
        
        std::optional<int> fcntl(int cmd, int arg) override;
        ErrnoOrBuffer ioctl(OpenFileDescription&, Ioctl request, const Buffer& buffer) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        std::optional<int> hostFileDescriptor() const override { return {}; }

        std::string className() const override;
    
    private:
        explicit PipeEndpoint(Pipe* pipe, PipeSide side, int flags) :
                pipe_(pipe), side_(side), flags_(flags) {
            (void)side_;
            (void)flags_;
        }
        Pipe* pipe_;
        PipeSide side_;
        int flags_;
    };

}

#endif