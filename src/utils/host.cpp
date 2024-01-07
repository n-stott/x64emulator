#include "utils/host.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>

Host& Host::the() {
    static Host instance;
    return instance;
}

std::optional<std::string> Host::filename(FD fd) const {
    if(auto it = openFiles_.find(fd.fd); it != openFiles_.end()) {
        return it->second;
    }
    return {};
}

f80 Host::round(f80 val) {
    // save control word
    long double x = f80::toLongDouble(val);
    unsigned short cw { 0 };
    asm volatile("fnstcw %0" : "=m" (cw));

    // set rounding mode to nearest
    unsigned short nearest = (unsigned short)(cw & ~(0x3 << 10));
    asm volatile("fldcw %0" :: "m" (nearest));

    // do the rounding
    asm volatile("fldt %0" :: "m"(x));
    asm volatile("frndint");
    asm volatile("fstpt %0" : "=m"(x));

    // load initial control word
    asm volatile("fldcw %0" :: "m" (cw));

    return f80::fromLongDouble(x);
}

Host::CPUID Host::cpuid(u32 a) {
    CPUID s;
    asm volatile("cpuid" : "=a" (s.a), "=b" (s.b), "=c" (s.c), "=d" (s.d) : "0" (a));
    if(a == 1) {
        // Pretend that the cpu does not know
        s.c &= ~(1 << 0  // SSE3
               | 1 << 9  // SSE3 extension
               | 1 << 19 // SSE4.1
               | 1 << 20 // SSE4.2
        );
    }
    return s;
}

Host::XGETBV Host::xgetbv(u32 c) {
    XGETBV s;
    asm volatile("mov %0, %%ecx" :: "r"(c));
    asm volatile("xgetbv" : "=a" (s.a), "=d" (s.d));
    return s;
}

std::optional<std::vector<u8>> Host::read(FD fd, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t nbytes = ::read(fd.fd, buffer.data(), count);
    if(nbytes < 0) return {};
    buffer.resize(nbytes);
    return std::optional(std::move(buffer));
}

ssize_t Host::write([[maybe_unused]] FD fd, [[maybe_unused]] const u8* data, size_t count) {
    const std::string red("\033[0;31m");
    const std::string reset("\033[0m");
    if(fd.fd == 1) {
        std::string s;
        s.resize(count+1, '\0');
        std::memcpy(s.data(), (const char*)data, count);
        std::cout << s << std::flush;
    } else if(fd.fd == 2) {
        std::string s;
        s.resize(count+1, '\0');
        std::memcpy(s.data(), (const char*)data, count);
        std::cout << red << s << reset << std::flush;
    } else {
        assert(false);
    }
    return (ssize_t)count;
}

int Host::close(FD fd) {
    int ret = ::close(fd.fd);
    if(ret >= 0) the().openFiles_.erase(fd.fd);
    return ret;
}

std::optional<std::vector<u8>> Host::stat(const std::string& path) {
    struct stat st;
    int rc = ::stat(path.c_str(), &st);
    if(rc < 0) return {};
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return std::optional(std::move(buf));
}

std::optional<std::vector<u8>> Host::fstat(FD fd) {
    struct stat st;
    int rc = ::fstat(fd.fd, &st);
    if(rc < 0) return {};
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return std::optional(std::move(buf));
}

off_t Host::lseek(FD fd, off_t offset, int whence) {
    off_t ret = ::lseek(fd.fd, offset, whence);
    if(ret < 0) return -errno;
    return ret;
}

Host::FD Host::openat(FD dirfd, const std::string& pathname, int flags, [[maybe_unused]] int mode) {
    assert((flags & O_ACCMODE) == O_RDONLY);
    if((flags & O_ACCMODE) != O_RDONLY) return FD{-1};
    int fd = ::openat(dirfd.fd, pathname.c_str(), O_RDONLY);
    if(fd >= 0) the().openFiles_[fd] = pathname;
    return FD{fd};
}

int Host::access(const std::string& path, int mode) {
    int ret = ::access(path.c_str(), mode);
    if(ret < 0) return -errno;
    return ret;
}

std::optional<std::vector<u8>> Host::readlink(const std::string& path, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t ret = ::readlink(path.c_str(), (char*)buffer.data(), buffer.size());
    if(ret < 0) return {};
    return std::optional(std::move(buffer));
}

std::optional<std::vector<u8>> Host::uname() {
    struct utsname buf;
    int ret = ::uname(&buf);
    if(ret < 0) return {};
    std::vector<u8> buffer;
    buffer.resize(sizeof(buf), 0x0);
    std::memcpy(buffer.data(), &buf, sizeof(buf));
    return std::optional(std::move(buffer));
}

std::optional<std::vector<u8>> Host::getcwd(size_t size) {
    std::vector<u8> path;
    path.resize(size, 0x0);
    char* cwd = ::getcwd((char*)path.data(), path.size());
    if(!cwd) return {};
    return std::optional(std::move(path));
}

std::vector<u8> Host::readFromFile(Host::FD fd, size_t length, off_t offset) {
    // mmap the file
    const u8* src = (const u8*)::mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd.fd, offset);
    if(src == (void*)MAP_FAILED) return {};

    // figure out size
    struct stat buf;
    if(::fstat(fd.fd, &buf) < 0) {
        return {};
    }
    size_t size = std::min((size_t)buf.st_size, (size_t)length);
    
    // copy data out
    std::vector<u8> data(src, src+size);
    
    // unmap the file
    if(::munmap((void*)src, length) < 0) {
        return {};
    }
    return data;
}