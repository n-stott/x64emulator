#include "kernel/fs/shadowfile.h"
#include "kernel/host.h"
#include "verify.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace kernel {

    template<typename Func>
    class ScopeGuard {
    public:
        ScopeGuard(Func&& func) : func_(func) { }
        ~ScopeGuard() {
            func_();
        }
    private:
        Func func_;
    };

    std::unique_ptr<ShadowFile> ShadowFile::tryCreate(FS* fs, const std::string& path, bool create) {
        int fd = ::openat(AT_FDCWD, path.c_str(), O_RDONLY | O_CLOEXEC);
        
        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        if(fd < 0) {
            if(!create) return {};
            std::vector<u8> data;
            return std::unique_ptr<ShadowFile>(new ShadowFile(fs, std::move(data)));
        } else {
            // figure out size
            struct stat buf;
            if(::fstat(fd, &buf) < 0) return {};
            
            // create data vector
            std::vector<u8> data((size_t)buf.st_size, 0x0);

            ssize_t nread = ::read(fd, data.data(), data.size());
            if(nread < 0) return {};
            if((size_t)nread != data.size()) return {};

            return std::unique_ptr<ShadowFile>(new ShadowFile(fs, std::move(data)));
        }
    }

    void ShadowFile::close() {
        
    }

    void ShadowFile::truncate() {
        data_.clear();
    }

    void ShadowFile::append() {
        verify(false, "ShadowFile::append() not implemented");
    }

    ErrnoOrBuffer ShadowFile::read(size_t count, off_t offset) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        if(offset < 0) return ErrnoOrBuffer{-EINVAL};
        size_t bytesRead = (size_t)offset < data_.size() ? std::min(data_.size() - (size_t)offset, count) : 0;
        const u8* beginRead = data_.data() + offset;
        const u8* endRead = beginRead + bytesRead;
        std::vector<u8> buffer(beginRead, endRead);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t ShadowFile::write(const u8* buf, size_t count, off_t offset) {
        if(!isWritable()) return -EINVAL;
        if(offset < 0) return -EINVAL;
        if(offset + count > data_.size()) {
            data_.resize(offset + count);
        }
        size_t bytesWritten = count;
        std::memcpy(data_.data() + offset, buf, bytesWritten);
        return (ssize_t)bytesWritten;
    }

    ErrnoOrBuffer ShadowFile::stat() {
        verify(false, "implement stat on ShadowFile");
        return ErrnoOrBuffer(-EINVAL);
    }

    off_t ShadowFile::lseek(off_t, int) {
        verify(false, "implement lseek on ShadowFile");
        return -EINVAL;
    }

    ErrnoOrBuffer ShadowFile::getdents64(size_t) {
        verify(false, "implement getdents64 on ShadowFile");
        return ErrnoOrBuffer(-EINVAL);
    }

    int ShadowFile::fcntl(int cmd, int arg) {
        if(cmd == F_SETLK) {
            warn([&]() { fmt::print(fg(fmt::color::red), "ShadowFile::fcntl({}, {}) not implemented\n", cmd, arg); });
            return 0;
        }
        verify(false, [&]() { fmt::print("ShadowFile::fcntl({}, {}) not implemented", cmd, arg); });
        return -EINVAL;
    }

    ErrnoOrBuffer ShadowFile::ioctl(unsigned long request, const Buffer&) {
        verify(false, [&]() { fmt::print("ShadowFile::ioctl({}) not implemented", request); });
        return ErrnoOrBuffer(-ENOTSUP);
    }

}