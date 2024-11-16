#include "kernel/fs/pipe.h"
#include "kernel/fs/openfiledescription.h"
#include "host/host.h"
#include "verify.h"
#include <algorithm>
#include <sys/errno.h>

namespace kernel {

    std::unique_ptr<Pipe> Pipe::tryCreate(FS* fs, int flags) {
        return std::unique_ptr<Pipe>(new Pipe(fs, flags));
    }

    std::unique_ptr<PipeEndpoint> Pipe::tryCreateReader() {
        auto ptr = PipeEndpoint::tryCreate(fs_, this, PipeSide::READ, flags_);
        if(!!ptr) readEndpoints_.push_back(ptr.get());
        return ptr;
    }

    std::unique_ptr<PipeEndpoint> Pipe::tryCreateWriter() {
        auto ptr = PipeEndpoint::tryCreate(fs_, this, PipeSide::WRITE, flags_);
        if(!!ptr) writeEndpoints_.push_back(ptr.get());
        return ptr;
    }

    void Pipe::closedEndpoint(const PipeEndpoint* endpoint) {
        readEndpoints_.erase(std::remove(readEndpoints_.begin(), readEndpoints_.end(), endpoint), readEndpoints_.end());
        writeEndpoints_.erase(std::remove(writeEndpoints_.begin(), writeEndpoints_.end(), endpoint), writeEndpoints_.end());
    }

    void Pipe::close() {
        verify(readEndpoints_.empty(), "cannot close pipe with active read endpoints");
        verify(writeEndpoints_.empty(), "cannot close pipe with active write endpoints");
    }

    std::unique_ptr<PipeEndpoint> PipeEndpoint::tryCreate(FS* fs, Pipe* pipe, PipeSide side, int flags) {
        return std::unique_ptr<PipeEndpoint>(new PipeEndpoint(fs, pipe, side, flags));
    }

    void PipeEndpoint::close() {
        pipe_->closedEndpoint(this);
    }

    bool Pipe::canRead() const {
        return !data_.empty();
    }

    bool Pipe::canWrite() const {
        (void)data_;
        return true;
    }

    ErrnoOrBuffer Pipe::read(OpenFileDescription& openFileDescription, size_t size) {
        bool nonBlocking = openFileDescription.statusFlags().test(FS::StatusFlags::NONBLOCK);
        if(data_.empty() && nonBlocking) {
            if(writeEndpoints_.empty()) {
                return ErrnoOrBuffer(Buffer{});
            } else {
                return ErrnoOrBuffer(-EAGAIN);
            }
        }
        verify(!data_.empty(), [&]() {
            fmt::print("Reading from blocking empty pipe not implemented\n");
            fmt::print("Pipe is non-blocking: {}\n", openFileDescription.statusFlags().test(FS::StatusFlags::NONBLOCK));
        });
        size_t readSize = std::min(size, data_.size());
        std::vector<u8> buf(readSize, 0x0);
        std::copy(data_.begin(), data_.begin() + (off64_t)readSize, buf.begin());
        data_.erase(data_.begin(), data_.begin() + (off64_t)readSize);
        return ErrnoOrBuffer(Buffer(std::move(buf)));
    }

    bool PipeEndpoint::canRead() const { return pipe_->canRead(); }
    bool PipeEndpoint::canWrite() const { return pipe_->canWrite(); }
    
    ErrnoOrBuffer PipeEndpoint::read(OpenFileDescription& openFileDescription, size_t size) {
        return pipe_->read(openFileDescription, size);
    }

    ssize_t Pipe::write(OpenFileDescription&, const u8* buf, size_t size) {
        data_.insert(data_.end(), buf, buf+size);
        return (ssize_t)size;
    }
    
    ssize_t PipeEndpoint::write(OpenFileDescription& openFileDescription, const u8* buf, size_t size) {
        return pipe_->write(openFileDescription, buf, size);
    }

    off_t PipeEndpoint::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "lseek not implemented on PipeEndpoint");
        return -ESPIPE;
    }

    ErrnoOrBuffer PipeEndpoint::stat() {
        verify(false, "stat not implemented on PipeEndpoint");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer PipeEndpoint::statfs() {
        verify(false, "statfs not implemented on PipeEndpoint");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer PipeEndpoint::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

    std::optional<int> PipeEndpoint::fcntl(int, int) {
        // nothing to do ?
        return {};
    }

    ErrnoOrBuffer PipeEndpoint::ioctl(unsigned long request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on PipeEndpoint", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

    std::string PipeEndpoint::className() const {
        if(side_ == PipeSide::READ) {
            return "Pipe Read Endpoint";
        } else {
            return "Pipe Write Endpoint";
        }
    }

}