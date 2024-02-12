#include "utils/host.h"
#include <cassert>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <asm/termbits.h>
#include <sys/auxv.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>

BufferOrErrno::BufferOrErrno(int err) : data_(err) { }

BufferOrErrno::BufferOrErrno(std::vector<u8> buf) : data_(std::move(buf)) { }

bool BufferOrErrno::isError() const {
    return std::holds_alternative<int>(data_);
}

bool BufferOrErrno::isBuffer() const {
    return !isError();
}

int BufferOrErrno::errorOr(int value) const {
    if(isError()) return std::get<int>(data_);
    return value;
}

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
        u32 sseMask = (u32)(1 << 0  // SSE3
                          | 1 << 9  // SSE3 extension
                          | 1 << 19 // SSE4.1
                          | 1 << 20 // SSE4.2
                          | 1 << 28 // AVX
                          );
        s.c = s.c & (~sseMask);
    }
    return s;
}

Host::XGETBV Host::xgetbv(u32 c) {
    XGETBV s;
    asm volatile("mov %0, %%ecx" :: "r"(c));
    asm volatile("xgetbv" : "=a" (s.a), "=d" (s.d));
    return s;
}

BufferOrErrno Host::read(FD fd, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t nbytes = ::read(fd.fd, buffer.data(), count);
    if(nbytes < 0) return BufferOrErrno(-errno);
    buffer.resize((size_t)nbytes);
    return BufferOrErrno(std::move(buffer));
}

ssize_t Host::write([[maybe_unused]] FD fd, [[maybe_unused]] const u8* data, size_t count) {
    if(fd.fd == 1 || fd.fd == 2) {
        std::string s;
        s.resize(count+1, '\0');
        std::memcpy(s.data(), (const char*)data, count);
        if(fd.fd == 1) {
            std::cout << s << std::flush;
        } else {
            std::cerr << s << std::flush;
        }
        return (ssize_t)count;
    } else {
        assert(false);
        return -EINVAL;
    }
}

int Host::close(FD fd) {
    int ret = ::close(fd.fd);
    if(ret >= 0) the().openFiles_.erase(fd.fd);
    return ret;
}

Host::FD Host::dup(FD oldfd) {
    int newfd = ::dup(oldfd.fd);
    if(newfd < 0) return FD{-errno};
    the().openFiles_[newfd] = the().openFiles_[oldfd.fd];
    return FD{newfd};
}

BufferOrErrno Host::stat(const std::string& path) {
    struct stat st;
    int rc = ::stat(path.c_str(), &st);
    if(rc < 0) return BufferOrErrno(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return BufferOrErrno(std::move(buf));
}

BufferOrErrno Host::fstat(FD fd) {
    struct stat st;
    int rc = ::fstat(fd.fd, &st);
    if(rc < 0) return BufferOrErrno(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return BufferOrErrno(std::move(buf));
}

BufferOrErrno Host::lstat(const std::string& path) {
    struct stat st;
    int rc = ::lstat(path.c_str(), &st);
    if(rc < 0) return BufferOrErrno(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return BufferOrErrno(std::move(buf));
}

off_t Host::lseek(FD fd, off_t offset, int whence) {
    off_t ret = ::lseek(fd.fd, offset, whence);
    if(ret < 0) return -errno;
    return ret;
}

Host::FD Host::openat(FD dirfd, const std::string& pathname, int flags, [[maybe_unused]] mode_t mode) {
    assert((flags & O_ACCMODE) == O_RDONLY);
    if((flags & O_ACCMODE) != O_RDONLY) return FD{-ENOTSUP};
    int fd = ::openat(dirfd.fd, pathname.c_str(), O_RDONLY | O_CLOEXEC);
    if(fd < 0) return FD{-errno};
    the().openFiles_[fd] = pathname;
    return FD{fd};
}

int Host::access(const std::string& path, int mode) {
    int ret = ::access(path.c_str(), mode);
    if(ret < 0) return -errno;
    return ret;
}

int Host::getfd(FD fd) {
    return ::fcntl(fd.fd, F_GETFD);
}

int Host::setfd(FD fd, int flag) {
    return ::fcntl(fd.fd, F_SETFD, flag);
}

BufferOrErrno Host::tcgetattr(FD fd) {
    struct termios ts;
    int ret = ::ioctl(fd.fd, TCGETS, &ts);
    if(ret < 0) return BufferOrErrno(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(ts), 0x0);
    std::memcpy(buffer.data(), &ts, sizeof(ts));
    return BufferOrErrno(std::move(buffer));
}

BufferOrErrno Host::tiocgwinsz(FD fd) {
    struct winsize ws;
    int ret = ::ioctl(fd.fd, TIOCGWINSZ, &ws);
    if(ret < 0) return BufferOrErrno(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(ws), 0x0);
    std::memcpy(buffer.data(), &ws, sizeof(ws));
    return BufferOrErrno(std::move(buffer));
}

BufferOrErrno Host::sysinfo() {
    struct sysinfo buf;
    int ret = ::sysinfo(&buf);
    if(ret < 0) return BufferOrErrno(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(struct sysinfo), 0x0);
    std::memcpy(buffer.data(), &buf, sizeof(buf));
    return BufferOrErrno(std::move(buffer));
}

int Host::getuid() {
    return ::getuid();
}

int Host::getgid() {
    return ::getgid();
}

int Host::geteuid() {
    return ::geteuid();
}

int Host::getegid() {
    return ::getegid();
}

BufferOrErrno Host::readlink(const std::string& path, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t ret = ::readlink(path.c_str(), (char*)buffer.data(), buffer.size());
    if(ret < 0) return BufferOrErrno(-errno);
    buffer.resize(ret);
    return BufferOrErrno(std::move(buffer));
}

BufferOrErrno Host::uname() {
    struct utsname buf;
    int ret = ::uname(&buf);
    if(ret < 0) return BufferOrErrno(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(buf), 0x0);
    std::memcpy(buffer.data(), &buf, sizeof(buf));
    return BufferOrErrno(std::move(buffer));
}

BufferOrErrno Host::getcwd(size_t size) {
    std::vector<u8> path;
    path.resize(size, 0x0);
    char* cwd = ::getcwd((char*)path.data(), path.size());
    if(!cwd) return BufferOrErrno(-errno);
    return BufferOrErrno(std::move(path));
}

BufferOrErrno Host::getdents64(FD fd, size_t count) {
    std::vector<u8> buf;
    buf.resize(count, 0x0);
    ssize_t nbytes = ::getdents64(fd.fd, buf.data(), buf.size());
    if(nbytes < 0) return BufferOrErrno(-errno);
    buf.resize(nbytes);
    return BufferOrErrno(std::move(buf));
}

BufferOrErrno Host::clock_gettime(clockid_t clockid) {
    timespec ts;
    int ret = ::clock_gettime(clockid, &ts);
    if(ret < 0) return BufferOrErrno(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(ts), 0x0);
    std::memcpy(buffer.data(), &ts, sizeof(ts));
    return BufferOrErrno(std::move(buffer));
}

time_t Host::time() {
    return ::time(nullptr);
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

int Host::prlimit64(pid_t pid, int resource, const std::vector<u8>* new_limit, std::vector<u8>* old_limit) {
    (void)pid;
    (void)new_limit;
    if(!old_limit) return 0;
    if(old_limit->size() != sizeof(struct rlimit)) return -1;
    switch(resource) {
        case RLIMIT_STACK: {
            struct rlimit stackLimit {
                16*0x1000,
                RLIM64_INFINITY,
            };
            std::memcpy(old_limit->data(), &stackLimit, sizeof(stackLimit));
            return 0;
        }
        default: {
            return -1;
        }
    }
}

std::optional<Host::AuxVal> Host::getauxval(AUX_TYPE type) {
    auto getDummy = [](u64 type) -> std::optional<AuxVal> {
        return AuxVal{type, 0};
    };
    auto get = [](u64 type) -> std::optional<AuxVal> {
        u64 res = ::getauxval(type);
        if(res == 0 && errno == ENOENT) return {};
        return AuxVal{type, res};
    };
    switch(type) {
        case AUX_TYPE::UID: return get(AT_UID);
        case AUX_TYPE::GID: return get(AT_GID);
        case AUX_TYPE::EUID: return get(AT_EUID);
        case AUX_TYPE::EGID: return get(AT_EGID);
        case AUX_TYPE::NIL: return getDummy(AT_NULL);
        case AUX_TYPE::ENTRYPOINT: return getDummy(AT_ENTRY);
        case AUX_TYPE::PROGRAM_HEADERS: return getDummy(AT_PHDR);
        case AUX_TYPE::PROGRAM_HEADER_ENTRY_SIZE: return getDummy(AT_PHENT);
        case AUX_TYPE::PROGRAM_HEADER_COUNT: return getDummy(AT_PHNUM);
        case AUX_TYPE::RANDOM_VALUE_ADDRESS: return getDummy(AT_RANDOM);
        case AUX_TYPE::PLATFORM_STRING_ADDRESS: return getDummy(AT_PLATFORM);
        case AUX_TYPE::VDSO_ADDRESS: return getDummy(AT_SYSINFO_EHDR);
        case AUX_TYPE::EXEC_FILE_DESCRIPTOR: return getDummy(AT_EXECFD);
        case AUX_TYPE::EXEC_PATH_NAME: return getDummy(AT_EXECFN);
    }
    return {};
}