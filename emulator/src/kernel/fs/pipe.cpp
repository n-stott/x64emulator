#include "kernel/fs/pipe.h"
#include "kernel/host.h"
#include "verify.h"
#include <sys/errno.h>

namespace kernel {

    std::unique_ptr<Pipe> Pipe::tryCreate(FS* fs, int flags) {
        return std::unique_ptr<Pipe>(new Pipe(fs, flags));
    }

    std::unique_ptr<PipeEndpoint> Pipe::tryCreateReader() {
        auto ptr = PipeEndpoint::tryCreate(fs_, this, PipeSide::READ, flags_);
        if(!!ptr) endpoints_.push_back(ptr.get());
        return ptr;
    }

    std::unique_ptr<PipeEndpoint> Pipe::tryCreateWriter() {
        auto ptr = PipeEndpoint::tryCreate(fs_, this, PipeSide::READ, flags_);
        if(!!ptr) endpoints_.push_back(ptr.get());
        return ptr;
    }

    void Pipe::close() {
        verify(endpoints_.empty(), "cannot close pipe with active endpoints");
    }

    std::unique_ptr<PipeEndpoint> PipeEndpoint::tryCreate(FS* fs, Pipe* pipe, PipeSide side, int flags) {
        return std::unique_ptr<PipeEndpoint>(new PipeEndpoint(fs, pipe, side, flags));
    }

    void PipeEndpoint::close() {
        (void)flags_;
    }

    ErrnoOrBuffer Pipe::read(size_t size) {
        verify(!data_.empty(), "Reading from empty pipe (possibly blocking) not implemented");
        size_t readSize = std::min(size, data_.size());
        std::vector<u8> buf(readSize, 0x0);
        std::copy(data_.begin(), data_.begin() + readSize, buf.begin());
        data_.erase(data_.begin(), data_.begin() + readSize);
        return ErrnoOrBuffer(Buffer(std::move(buf)));
    }
    
    ErrnoOrBuffer PipeEndpoint::read(size_t size, off_t) {
        return pipe_->read(size);
    }

    ssize_t Pipe::write(const u8* buf, size_t size) {
        data_.insert(data_.end(), buf, buf+size);
        return (ssize_t)size;
    }
    
    ssize_t PipeEndpoint::write(const u8* buf, size_t size, off_t) {
        return pipe_->write(buf, size);
    }

    off_t PipeEndpoint::lseek(off_t, int) {
        verify(false, "lseek not implemented on PipeEndpoint");
        return -ESPIPE;
    }

    ErrnoOrBuffer PipeEndpoint::stat() {
        verify(false, "stat not implemented on PipeEndpoint");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer PipeEndpoint::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

    int PipeEndpoint::fcntl(int cmd, int arg) {
        if(Host::Fcntl::isGetFd(cmd)) {
            // TODO
            return 0;
        }
        if(Host::Fcntl::isSetFd(cmd)) {
            // TODO
            return 0;
        }
        if(Host::Fcntl::isGetFl(cmd)) {
            // TODO
            return 0;
        }
        if(Host::Fcntl::isSetFl(cmd)) {
            // TODO
            return 0;
        }
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on PipeEndpoint", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer PipeEndpoint::ioctl(unsigned long request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on PipeEndpoint", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}