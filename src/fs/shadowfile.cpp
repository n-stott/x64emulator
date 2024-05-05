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

    ssize_t ShadowFile::read(u8* buf, size_t count) {
        if(!isReadable()) return -EINVAL;
        if(offset_ == data_.size()) return 0;
        size_t bytesRead = std::min(data_.size() - offset_, count);
        std::memcpy(buf, data_.data() + offset_, bytesRead);
        offset_ += bytesRead;
        return bytesRead;
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

}