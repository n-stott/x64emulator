#include "utils/host.h"
#include <cassert>
#include <cstring>
#include <dirent.h>
#include <poll.h>
#include <asm/prctl.h>
#include <asm/termbits.h>
#include <sys/auxv.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

Host::Host() {
    openFiles_[0] = "___stdin";
    openFiles_[1] = "___stdout";
    openFiles_[2] = "___stderr";
}

Host::~Host() = default;

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

Host::IdivResult<u32> Host::idiv32(u32 upperDividend, u32 lowerDividend, u32 divisor) {
    u32 quotient;
    u32 remainder;
    asm volatile("mov %2, %%edx\n"
                 "mov %3, %%eax\n"
                 "idiv %4\n"
                 "mov %%edx, %0\n"
                 "mov %%eax, %1\n" : "=r"(remainder), "=r"(quotient)
                                 : "r"(upperDividend), "r"(lowerDividend), "r"(divisor)
                                 : "eax", "edx");
    return IdivResult<u32>{quotient, remainder};
}

Host::IdivResult<u64> Host::idiv64(u64 upperDividend, u64 lowerDividend, u64 divisor) {
    u64 quotient;
    u64 remainder;
    asm volatile("mov %2, %%rdx\n"
                 "mov %3, %%rax\n"
                 "idiv %4\n"
                 "mov %%rdx, %0\n"
                 "mov %%rax, %1\n" : "=r"(remainder), "=r"(quotient)
                                 : "r"(upperDividend), "r"(lowerDividend), "r"(divisor)
                                 : "rax", "rdx");
    return IdivResult<u64>{quotient, remainder};
}

Host::CPUID Host::cpuid(u32 a, u32 c) {
    CPUID s;
    s.a = a;
    s.c = c;
    s.b = s.d = 0;

    asm volatile ("xchgq %%rbx, %q1\n"
                  "cpuid\n"
                  "xchgq %%rbx, %q1\n" : "=a" (s.a), "=r" (s.b), "=c" (s.c), "=d" (s.d)
                                       : "0" (a), "2" (c));

    if(a == 1) {
        // Pretend that we run on cpu 0
        s.b = s.b & 0x00FFFFFF;
        // Pretend that the cpu does not know
        u32 mask = (u32)(1 << 0  // SSE3
                       | 1 << 9  // SSE3 extension
                       | 1 << 19 // SSE4.1
                       | 1 << 20 // SSE4.2
                       | 1 << 26 // xsave
                       | 1 << 27 // xsave by os
                       | 1 << 28 // AVX
                       | 1 << 30 // RDRAND
                       );
        s.c = s.c & (~mask);
    }
    if(a == 7 && c == 0) {
        // Pretend that we do not have
        u32 mask = (u32)(1 << 7 // Control flow enforcement: shadow stack
                        );
        s.c = s.c & (~mask);
    }
    return s;
}

Host::XGETBV Host::xgetbv(u32 c) {
    XGETBV s;
    asm volatile("mov %0, %%ecx" :: "r"(c));
    asm volatile("xgetbv" : "=a" (s.a), "=d" (s.d));
    return s;
}


bool Host::Mmap::isAnonymous(int flags) {
    return flags & MAP_ANONYMOUS;
}

bool Host::Mmap::isFixed(int flags) {
    return flags & MAP_FIXED;
}

bool Host::Prctl::isSetFS(int code) {
    return code == ARCH_SET_FS;
}

ErrnoOrBuffer Host::read(FD fd, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t nbytes = ::read(fd.fd, buffer.data(), count);
    if(nbytes < 0) return ErrnoOrBuffer(-errno);
    buffer.resize((size_t)nbytes);
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

ErrnoOrBuffer Host::pread64(FD fd, size_t count, off_t offset) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t nbytes = ::pread(fd.fd, buffer.data(), count, offset);
    if(nbytes < 0) return ErrnoOrBuffer(-errno);
    buffer.resize((size_t)nbytes);
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

ssize_t Host::write(FD fd, [[maybe_unused]] const u8* data, size_t count) {
    ssize_t ret = ::write(fd.fd, data, count);
    if(ret < 0) return (ssize_t)(-errno);
    return ret;
}

ssize_t Host::pwrite64(FD fd, const u8* data, size_t count, off_t offset) {
    ssize_t ret = ::pwrite(fd.fd, data, count, offset);
    if(ret < 0) return (ssize_t)(-errno);
    return ret;
}

int Host::close(FD fd) {
    int ret = ::close(fd.fd);
    if(ret >= 0) openFiles_.erase(fd.fd);
    return ret;
}

Host::FD Host::dup(FD oldfd) {
    int newfd = ::dup(oldfd.fd);
    if(newfd < 0) return FD{-errno};
    openFiles_[newfd] = openFiles_[oldfd.fd];
    return FD{newfd};
}


size_t Host::iovecRequiredBufferSize() {
    return sizeof(iovec);
}

size_t Host::iovecLen(const Buffer& buffer, size_t i) {
    assert(i*sizeof(iovec) < buffer.size());
    return ((const iovec*)buffer.data() + i)->iov_len;
}

u64 Host::iovecBase(const Buffer& buffer, size_t i) {
    assert(i*sizeof(iovec) < buffer.size());
    return (u64)((const iovec*)buffer.data() + i)->iov_base;
}

ssize_t Host::writev(FD fd, const std::vector<Buffer>& buffers) {
    std::vector<iovec> iovecs;
    for(const auto& buffer : buffers) {
        iovecs.push_back(iovec{(void*)buffer.data(), buffer.size()});
    }
    ssize_t nbytes = ::writev(fd.fd, iovecs.data(), (int)iovecs.size());
    if(nbytes < 0) return -errno;
    return nbytes;
}

ErrnoOrBuffer Host::stat(const std::string& path) {
    struct stat st;
    int rc = ::stat(path.c_str(), &st);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::fstat(FD fd) {
    struct stat st;
    int rc = ::fstat(fd.fd, &st);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::lstat(const std::string& path) {
    struct stat st;
    int rc = ::lstat(path.c_str(), &st);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::fstatat64(FD dirfd, const std::string& path, int flags) {
    struct stat st;
    int rc = ::fstatat(dirfd.fd, path.c_str(), &st, flags);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(st), 0x0);
    std::memcpy(buf.data(), &st, sizeof(st));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

off_t Host::lseek(FD fd, off_t offset, int whence) {
    off_t ret = ::lseek(fd.fd, offset, whence);
    if(ret < 0) return -errno;
    return ret;
}

Host::FD Host::openat(FD dirfd, const std::string& pathname, int flags, [[maybe_unused]] mode_t mode) {
    flags = (flags & ~O_ACCMODE) | O_RDONLY | O_CLOEXEC;
    flags = (flags & ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC));
    int fd = ::openat(dirfd.fd, pathname.c_str(), flags);
    if(fd < 0) return FD{-errno};
    openFiles_[fd] = pathname;
    return FD{fd};
}

int Host::access(const std::string& path, int mode) {
    int ret = ::access(path.c_str(), mode);
    if(ret < 0) return -errno;
    return ret;
}

ErrnoOrBuffer Host::statfs(const std::string& path) {
    struct statfs stfs;
    int rc = ::statfs(path.c_str(), &stfs);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(stfs), 0x0);
    std::memcpy(buf.data(), &stfs, sizeof(stfs));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::statx(FD dirfd, const std::string& path, int flags, unsigned int mask) {
    struct statx sx;
    int rc = ::statx(dirfd.fd, path.c_str(), flags, mask, &sx);
    if(rc < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buf(sizeof(sx), 0x0);
    std::memcpy(buf.data(), &sx, sizeof(sx));
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::getxattr(const std::string& path, const std::string& name, size_t size) {
    std::vector<u8> buf(size, 0x0);
    ssize_t ret = ::getxattr(path.c_str(), name.c_str(), buf.data(), size);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    buf.resize((size_t)ret);
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

ErrnoOrBuffer Host::lgetxattr(const std::string& path, const std::string& name, size_t size) {
    std::vector<u8> buf(size, 0x0);
    ssize_t ret = ::lgetxattr(path.c_str(), name.c_str(), buf.data(), size);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    buf.resize((size_t)ret);
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

std::string Host::fcntlName(int cmd) {
    switch(cmd) {
        case F_GETFD: return "F_GETFD";
        case F_SETFD: return "F_SETFD";
        case F_GETFL: return "F_GETFL";
        case F_SETFL: return "F_SETFL";
    }
    return "unknown fcntl " + std::to_string(cmd);
}

int Host::fcntl(FD fd, int cmd, int arg) {
    switch(cmd) {
        case F_GETFD:
        case F_SETFD:
        case F_GETFL:
        case F_SETFL: {
            int ret = ::fcntl(fd.fd, cmd, arg);
            if(ret < 0) return -errno;
            return ret;
        }
    }
    return -ENOTSUP;
}

Host::FD Host::socket(int domain, int type, int protocol) {
    auto validateDomain = [](int domain) -> bool {
        if(domain == AF_UNIX) return true;
        if(domain == AF_LOCAL) return true;
        if(domain == AF_NETLINK) return true;
        return false;
    };
    if(!validateDomain(domain)) return FD{-ENOTSUP};
    int fd = ::socket(domain, type, protocol);
    if(fd < 0) return FD{-errno};
    return FD{fd};
}

int Host::connect(int sockfd, const Buffer& addr) {
    int ret = ::connect(sockfd, (const struct sockaddr*)addr.data(), (socklen_t)addr.size());
    if(ret < 0) return -errno;
    return ret;
}

ErrnoOrBuffer Host::getsockname(int sockfd, u32 buffersize) {
    static_assert(sizeof(socklen_t) == sizeof(u32));
    std::vector<u8> buffer;
    buffer.resize(buffersize, 0x0);
    int ret = ::getsockname(sockfd, (sockaddr*)buffer.data(), &buffersize);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    buffer.resize((size_t)buffersize);
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

ErrnoOrBuffer Host::getpeername(int sockfd, u32 buffersize) {
    static_assert(sizeof(socklen_t) == sizeof(u32));
    std::vector<u8> buffer;
    buffer.resize(buffersize, 0x0);
    int ret = ::getpeername(sockfd, (sockaddr*)buffer.data(), &buffersize);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    buffer.resize((size_t)buffersize);
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

int Host::bind(FD sockfd, const Buffer& addr) {
    int ret = ::bind(sockfd.fd, (const struct sockaddr*)addr.data(), (socklen_t)addr.size());
    if(ret < 0) return -errno;
    return ret;
}

ErrnoOr<std::pair<Buffer, Buffer>> Host::recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress) {
    static_assert(sizeof(socklen_t) == sizeof(u32));
    if(requireSrcAddress) {
        return ErrnoOr<std::pair<Buffer, Buffer>>(-ENOTSUP);
    }
    std::vector<u8> buffer;
    buffer.resize(len, 0);
    ssize_t ret = ::recvfrom(sockfd.fd, buffer.data(), len, flags, nullptr, nullptr);
    if(ret < 0) return ErrnoOr<std::pair<Buffer, Buffer>>(-errno);
    buffer.resize((size_t)ret);
    return ErrnoOr<std::pair<Buffer, Buffer>>(std::make_pair(Buffer{std::move(buffer)}, Buffer{}));
}

ssize_t Host::recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags) {
    // struct msghdr {
    //     void*         msg_name;       /* Optional address */
    //     socklen_t     msg_namelen;    /* Size of address */
    //     struct iovec* msg_iov;        /* Scatter/gather array */
    //     size_t        msg_iovlen;     /* # elements in msg_iov */
    //     void*         msg_control;    /* Ancillary data, see below */
    //     size_t        msg_controllen; /* Ancillary data buffer len */
    //     int           msg_flags;      /* Flags on received message */
    // };
    msghdr header;
    header.msg_name = msg_name->data();
    header.msg_namelen = (socklen_t)msg_name->size();
    std::vector<iovec> iovs;
    for(auto& buf : *msg_iov) {
        iovec iov;
        iov.iov_base = buf.data();
        iov.iov_len = buf.size();
        iovs.push_back(iov);
    }
    header.msg_iov = iovs.data();
    header.msg_iovlen = iovs.size();
    header.msg_control = msg_control->data();
    header.msg_controllen = msg_control->size();
    header.msg_flags = 0;
    ssize_t ret = ::recvmsg(sockfd.fd, &header, flags);
    *msg_flags = header.msg_flags;
    if(ret < 0) return -errno;
    return ret;
}

ssize_t Host::sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags) {
    // struct msghdr {
    //     void*         msg_name;       /* Optional address */
    //     socklen_t     msg_namelen;    /* Size of address */
    //     struct iovec* msg_iov;        /* Scatter/gather array */
    //     size_t        msg_iovlen;     /* # elements in msg_iov */
    //     void*         msg_control;    /* Ancillary data, see below */
    //     size_t        msg_controllen; /* Ancillary data buffer len */
    //     int           msg_flags;      /* Flags on received message */
    // };
    msghdr header;
    header.msg_name = (void*)msg_name.data();
    header.msg_namelen = (socklen_t)msg_name.size();
    std::vector<iovec> iovs;
    for(auto& buf : msg_iov) {
        iovec iov;
        iov.iov_base = (void*)buf.data();
        iov.iov_len = buf.size();
        iovs.push_back(iov);
    }
    header.msg_iov = iovs.data();
    header.msg_iovlen = iovs.size();
    header.msg_control = (void*)msg_control.data();
    header.msg_controllen = msg_control.size();
    header.msg_flags = msg_flags;
    ssize_t ret = ::sendmsg(sockfd.fd, &header, flags);
    if(ret < 0) return -errno;
    return ret;
}

std::string Host::ioctlName(unsigned long request) {
    switch(request) {
        case TCGETS: return "TCGETS";
        case FIOCLEX: return "FIOCLEX";
        case FIONCLEX: return "FIONCLEX";
        case TIOCGWINSZ: return "TIOCGWINSZ";
        case TIOCSWINSZ: return "TIOCSWINSZ";
    }
    return "unknown ioctl";
}

size_t Host::ioctlRequiredBufferSize(unsigned long request) {
    switch(request) {
        case TCGETS:
        case FIOCLEX:
        case FIONCLEX:
        case TIOCGWINSZ: return 0;
        case TIOCSWINSZ: return sizeof(winsize);
        case TCSETSW: return sizeof(termios);
    }
    return 0;
}

ErrnoOrBuffer Host::ioctl(FD fd, unsigned long request, [[maybe_unused]] const Buffer& inputBuffer) {
    switch(request) {
        case TCGETS: {
            struct termios ts;
            int ret = ::ioctl(fd.fd, TCGETS, &ts);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            std::vector<u8> buffer;
            buffer.resize(sizeof(ts), 0x0);
            std::memcpy(buffer.data(), &ts, sizeof(ts));
            return ErrnoOrBuffer(Buffer{std::move(buffer)});
        }
        case FIOCLEX: {
            int ret = ::ioctl(fd.fd, FIOCLEX, nullptr);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            return ErrnoOrBuffer(Buffer{});
        }
        case FIONCLEX: {
            int ret = ::ioctl(fd.fd, FIONCLEX, nullptr);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            return ErrnoOrBuffer(Buffer{});
        }
        case TIOCGWINSZ: {
            struct winsize ws;
            int ret = ::ioctl(fd.fd, TIOCGWINSZ, &ws);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            std::vector<u8> buffer;
            buffer.resize(sizeof(ws), 0x0);
            std::memcpy(buffer.data(), &ws, sizeof(ws));
            return ErrnoOrBuffer(Buffer{std::move(buffer)});
        }
        case TIOCSWINSZ: {
            struct winsize ws;
            std::memcpy(&ws, inputBuffer.data(), sizeof(ws));
            int ret = ::ioctl(fd.fd, TIOCSWINSZ, &ws);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            return ErrnoOrBuffer(Buffer{});
        }
        case TCSETSW: {
            struct termios ts;
            std::memcpy(&ts, inputBuffer.data(), sizeof(ts));
            int ret = ::ioctl(fd.fd, TCSETSW, &ts);
            if(ret < 0) return ErrnoOrBuffer(-errno);
            return ErrnoOrBuffer(Buffer{});
        }
    }
    return ErrnoOrBuffer(-ENOTSUP);
}

ErrnoOrBuffer Host::sysinfo() {
    struct sysinfo buf;
    int ret = ::sysinfo(&buf);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(struct sysinfo), 0x0);
    std::memcpy(buffer.data(), &buf, sizeof(buf));
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

int Host::getuid() {
    return (int)::getuid();
}

int Host::getgid() {
    return (int)::getgid();
}

int Host::geteuid() {
    return (int)::geteuid();
}

int Host::getegid() {
    return (int)::getegid();
}

int Host::getpgrp() {
    return (int)::getpgrp();
}

ErrnoOrBuffer Host::readlink(const std::string& path, size_t count) {
    std::vector<u8> buffer;
    buffer.resize(count, 0x0);
    ssize_t ret = ::readlink(path.c_str(), (char*)buffer.data(), buffer.size());
    if(ret < 0) return ErrnoOrBuffer(-errno);
    buffer.resize((size_t)ret);
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

ErrnoOrBuffer Host::uname() {
    struct utsname buf;
    int ret = ::uname(&buf);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(buf), 0x0);
    std::memcpy(buffer.data(), &buf, sizeof(buf));
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

ErrnoOrBuffer Host::getcwd(size_t size) {
    std::vector<u8> path;
    path.resize(size, 0x0);
    char* cwd = ::getcwd((char*)path.data(), path.size());
    if(!cwd) return ErrnoOrBuffer(-errno);
    return ErrnoOrBuffer(Buffer{std::move(path)});
}

ErrnoOrBuffer Host::getdents64(FD fd, size_t count) {
    std::vector<u8> buf;
    buf.resize(count, 0x0);
    ssize_t nbytes = ::getdents64(fd.fd, buf.data(), buf.size());
    if(nbytes < 0) return ErrnoOrBuffer(-errno);
    buf.resize((size_t)nbytes);
    return ErrnoOrBuffer(Buffer{std::move(buf)});
}

int Host::chdir(const std::string& path) {
    return ::chdir(path.c_str());
}

ErrnoOrBuffer Host::clock_gettime(clockid_t clockid) {
    timespec ts;
    int ret = ::clock_gettime(clockid, &ts);
    if(ret < 0) return ErrnoOrBuffer(-errno);
    std::vector<u8> buffer;
    buffer.resize(sizeof(ts), 0x0);
    std::memcpy(buffer.data(), &ts, sizeof(ts));
    return ErrnoOrBuffer(Buffer{std::move(buffer)});
}

time_t Host::time() {
    return ::time(nullptr);
}

ErrnoOr<std::pair<Buffer, Buffer>> Host::gettimeofday() {
    using ReturnType = ErrnoOr<std::pair<Buffer, Buffer>>;
    struct timeval tv;
    struct timezone tz;
    int rc = ::gettimeofday(&tv, &tz);
    if(rc < 0) return ReturnType(-errno);
    Buffer v(tv);
    Buffer z(tz);
    return ReturnType(std::make_pair(std::move(v), std::move(z)));
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

ErrnoOrBuffer Host::getrlimit([[maybe_unused]] pid_t pid, int resource) {
    switch(resource) {
        case RLIMIT_STACK: {
            struct rlimit stackLimit {
                16*0x1000,
                RLIM64_INFINITY,
            };
            return ErrnoOrBuffer(Buffer{stackLimit});
        }
        default: {
            return ErrnoOrBuffer(-ENOTSUP);
        }
    }
}


size_t Host::pollRequiredBufferSize(size_t nfds) {
    return sizeof(pollfd)*nfds;
}

ErrnoOr<BufferAndReturnValue<int>> Host::poll(const Buffer& buffer, u64 nfds, int timeout) {
    std::vector<pollfd> pollfds;
    assert(buffer.size() % sizeof(pollfd) == 0);
    pollfds.resize(buffer.size() / sizeof(pollfd));
    std::memcpy(pollfds.data(), buffer.data(), buffer.size());
    int ret = ::poll(pollfds.data(), nfds, timeout);
    if(ret < 0) return ErrnoOr<BufferAndReturnValue<int>>(-errno);
    BufferAndReturnValue<int> bufferAndRetVal {
        Buffer{std::move(pollfds)},
        ret,
    };
    return ErrnoOr<BufferAndReturnValue<int>>(std::move(bufferAndRetVal));
}

Host::FD Host::epoll_create1(int flags) {
    int fd = ::epoll_create1(flags);
    if(fd < 0) return FD{-errno};
    return FD{fd};
}

Host::FD Host::eventfd2(unsigned int initval, int flags) {
    int fd = ::eventfd(initval, flags);
    if(fd < 0) return FD{-errno};
    return FD{fd};
}

int Host::select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval* timeout) {
    int ret = ::select(nfds, readfds, writefds, exceptfds, timeout);
    if(ret < 0) return -errno;
    return ret;
}

int Host::pselect6(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timespec* timeout, const sigset_t* sigmask) {
    int ret = ::pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
    if(ret < 0) return -errno;
    return ret;
}

std::optional<Host::AuxVal> Host::getauxval(AUX_TYPE type) {
    auto getDummy = [](u64 type) -> std::optional<AuxVal> {
        return AuxVal{type, 0};
    };
    auto get = [](u64 type) -> std::optional<AuxVal> {
        errno = 0;
        u64 res = ::getauxval(type);
        if(res == 0 && errno == ENOENT) return {};
        return AuxVal{type, res};
    };
    switch(type) {
        case AUX_TYPE::UID: return get(AT_UID);
        case AUX_TYPE::GID: return get(AT_GID);
        case AUX_TYPE::EUID: return get(AT_EUID);
        case AUX_TYPE::EGID: return get(AT_EGID);
        case AUX_TYPE::SECURE: return get(AT_SECURE);
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