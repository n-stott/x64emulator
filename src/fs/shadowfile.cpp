#include "fs/shadowfile.h"
#include "host/host.h"
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
            std::vector<u8> data(buf.st_size, 0x0);

            ssize_t nread = ::read(fd, data.data(), data.size());
            if(nread < 0) return {};
            if((size_t)nread != data.size()) return {};

            return std::unique_ptr<ShadowFile>(new ShadowFile(fs, std::move(data)));
        }
    }

    void ShadowFile::truncate() {
        data_.clear();
        offset_ = 0;
    }

    void ShadowFile::append() {
        offset_ = data_.size();
    }

    ErrnoOrBuffer ShadowFile::read(size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        size_t bytesRead = offset_ < data_.size() ? std::min(data_.size() - offset_, count) : 0;
        const u8* beginRead = data_.data() + offset_;
        const u8* endRead = beginRead + bytesRead;
        std::vector<u8> buffer(beginRead, endRead);
        offset_ += bytesRead;
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t ShadowFile::write(const u8* buf, size_t count) {
        if(!isWritable()) return -EINVAL;
        if(offset_ + count > data_.size()) {
            data_.resize(offset_ + count);
        }
        size_t bytesWritten = count;
        std::memcpy(data_.data() + offset_, buf, bytesWritten);
        offset_ += bytesWritten;
        return bytesWritten;
    }

    ErrnoOrBuffer ShadowFile::pread(size_t count, size_t offset) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        size_t bytesRead = offset < data_.size() ? std::min(data_.size() - offset, count) : 0;
        const u8* beginRead = data_.data() + offset;
        const u8* endRead = beginRead + bytesRead;
        std::vector<u8> buffer(beginRead, endRead);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t ShadowFile::pwrite(const u8*, size_t, size_t) {
        if(!isWritable()) return -EINVAL;
        return -ENOTSUP;
    }

}