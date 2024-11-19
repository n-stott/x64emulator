#include "host/host.h"
#include "scopeguard.h"
#include <cassert>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <poll.h>
#include <asm/prctl.h>
#include <asm/termbits.h>
#include <sched.h>
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

namespace kernel {

    Host::Host() = default;
    Host::~Host() = default; // NOLINT(performance-trivially-destructible)

    bool Host::Mmap::isAnonymous(int flags) {
        return flags & MAP_ANONYMOUS;
    }

    bool Host::Mmap::isFixed(int flags) {
        return flags & MAP_FIXED;
    }

    bool Host::Mmap::isPrivate(int flags) {
        return flags & MAP_PRIVATE;
    }

    bool Host::Mmap::isShared(int flags) {
        return flags & MAP_SHARED;
    }
    
    bool Host::Madvise::isDontNeed(int advice) {
        return advice == MADV_DONTNEED;
    }

    bool Host::Prctl::isSetFS(int code) {
        return code == ARCH_SET_FS;
    }


    bool Host::Open::isReadable(int flag) {
        return (flag & O_ACCMODE) == O_RDONLY;
    }

    bool Host::Open::isWritable(int flag) {
        return (flag & O_ACCMODE) & (O_WRONLY | O_RDWR);
    }

    bool Host::Open::isAppending(int flag) {
        return flag & O_APPEND;
    }

    bool Host::Open::isTruncating(int flag) {
        return flag & O_TRUNC;
    }

    bool Host::Open::isCreatable(int flag) {
        return flag & O_CREAT;
    }
    
    bool Host::Open::isCloseOnExec(int flag) {
        return flag & O_CLOEXEC;
    }
    
    bool Host::Open::isDirectory(int flag) {
        return flag & O_DIRECTORY;
    }

    bool Host::Open::isLargeFile(int flag) {
        return flag & O_LARGEFILE;
    }

    bool Host::Open::isNonBlock(int flag) {
        return flag & O_NONBLOCK;
    }

    bool Host::Fcntl::isGetFd(int cmd) {
        return cmd == F_GETFD;
    }

    bool Host::Fcntl::isSetFd(int cmd) {
        return cmd == F_SETFD;
    }

    bool Host::Fcntl::isGetFl(int cmd) {
        return cmd == F_GETFL;
    }

    bool Host::Fcntl::isSetFl(int cmd) {
        return cmd == F_SETFL;
    }

    bool Host::Mode::isUserReadable(unsigned int mode) {
        return mode & S_IRUSR;
    }

    bool Host::Mode::isUserWritable(unsigned int mode) {
        return mode & S_IWUSR;
    }

    bool Host::Mode::isUserExecutable(unsigned int mode) {
        return mode & S_IXUSR;
    }

    bool Host::FallocateMode::isKeepSize(int mode) {
        return mode & FALLOC_FL_KEEP_SIZE;
    }

    bool Host::FallocateMode::isPunchHole(int mode) {
        return mode & FALLOC_FL_PUNCH_HOLE;
    }

    bool Host::FallocateMode::isNoHidestale(int mode) {
        return mode & FALLOC_FL_NO_HIDE_STALE;
    }

    bool Host::FallocateMode::isCollapseRange(int mode) {
        return mode & FALLOC_FL_COLLAPSE_RANGE;
    }

    bool Host::FallocateMode::isZeroRange(int mode) {
        return mode & FALLOC_FL_ZERO_RANGE;
    }

    bool Host::FallocateMode::isInsertRange(int mode) {
        return mode & FALLOC_FL_INSERT_RANGE;
    }

    bool Host::FallocateMode::isUnshareRange(int mode) {
        return mode & FALLOC_FL_UNSHARE_RANGE;
    }

    bool Host::MemfdFlags::isCloseOnExec(unsigned int flags) {
        return flags & MFD_CLOEXEC;
    }

    bool Host::MemfdFlags::allowsSealing(unsigned int flags) {
        return flags & MFD_ALLOW_SEALING;
    }

    bool Host::MemfdFlags::isOther(unsigned int flags) {
        return flags & ~(MFD_CLOEXEC | MFD_ALLOW_SEALING);
    }

    bool Host::Eventfd2Flags::isCloseOnExec(int flags) {
        return flags & EFD_CLOEXEC;
    }

    bool Host::Eventfd2Flags::isNonBlock(int flags) {
        return flags & EFD_NONBLOCK;
    }

    bool Host::Eventfd2Flags::isOther(int flags) {
        return flags & ~(EFD_CLOEXEC | EFD_NONBLOCK);
    }

    bool Host::EpollFlags::isCloseOnExec(int flags) {
        return flags & EPOLL_CLOEXEC;
    }

    bool Host::EpollFlags::isOther(int flags) {
        return flags & ~(EPOLL_CLOEXEC);
    }

    bool Host::SocketType::isCloseOnExec(int type) {
        return type & SOCK_CLOEXEC;
    }

    bool Host::SocketType::isNonBlock(int type) {
        return type & SOCK_NONBLOCK;
    }
    
    bool Host::Lseek::isSeekSet(int whence) {
        return whence == SEEK_SET;
    }
    
    bool Host::Lseek::isSeekCur(int whence) {
        return whence == SEEK_CUR;
    }
    
    bool Host::Lseek::isSeekEnd(int whence) {
        return whence == SEEK_END;
    }

    Host::SchedAttr Host::getSchedulerAttributes() {
        Host::SchedAttr attr;
        attr.size = sizeof(Host::SchedAttr);
        attr.schedPolicy = SCHED_OTHER;
        attr.schedFlags = 0;
        attr.schedNice = 0;
        attr.schedPriority = 0;
        attr.schedRuntime = 0;
        attr.schedDeadline = 0;
        attr.schedPeriod = 0;
        return attr;
    }

    Host::CloneFlags Host::fromCloneFlags(unsigned long flags) {
        CloneFlags cloneFlags;
        cloneFlags.childClearTid = flags & CLONE_CHILD_CLEARTID;
        cloneFlags.childSetTid = flags & CLONE_CHILD_SETTID;
        cloneFlags.clearSignalHandlers = false; // unsupported on my machine ?
        cloneFlags.cloneSignalHandlers = flags & CLONE_SIGHAND;
        cloneFlags.cloneFiles = flags & CLONE_FILES;
        cloneFlags.cloneFs = flags & CLONE_FS;
        cloneFlags.cloneIo = flags & CLONE_IO;
        cloneFlags.cloneParent = flags & CLONE_PARENT;
        cloneFlags.parentSetTid = flags & CLONE_PARENT_SETTID;
        cloneFlags.clonePidFd = flags & CLONE_PIDFD;
        cloneFlags.setTls = flags & CLONE_SETTLS;
        cloneFlags.cloneThread = flags & CLONE_THREAD;
        cloneFlags.cloneVm = flags & CLONE_VM;
        cloneFlags.cloneVfork = flags & CLONE_VFORK;
        return cloneFlags;
    }

    Host::FD Host::cwdfd() {
        return FD{AT_FDCWD};
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
            case F_SETLK: return "F_SETLK";
            case F_SETLKW: return "F_SETLKW";
            case F_DUPFD: return "F_DUPFD";
            case F_DUPFD_CLOEXEC: return "F_DUPFD_CLOEXEC";
            default: break;
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
            default: break;
        }
        return -ENOTSUP;
    }

    std::string Host::ioctlName(unsigned long request) {
        switch(request) {
            case TCGETS: return "TCGETS";
            case FIOCLEX: return "FIOCLEX";
            case FIONCLEX: return "FIONCLEX";
            case TIOCGWINSZ: return "TIOCGWINSZ";
            case TIOCSWINSZ: return "TIOCSWINSZ";
            case TIOCGPGRP: return "TIOCGPGRP";
            default: break;
        }
        std::stringstream ss;
        ss << "Unknown ioctl 0x" << std::hex << request;
        return ss.str();
    }

    std::optional<ssize_t> Host::tryGuessIoctlBufferSize(FD fd, unsigned long request, const u8* data, size_t size) {

        class PageBoundary {
        public:
            static std::optional<PageBoundary> tryCreate() {
                const size_t PAGE_SIZE = 0x1000;
                char* ptr = (char*)::mmap(nullptr, 2*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
                if(ptr != (char*)MAP_FAILED) {
                    return std::optional<PageBoundary>(PageBoundary(ptr));
                } else {
                    return {};
                }
            }

            PageBoundary(PageBoundary&& pb) noexcept : ptr_(pb.ptr_) {
                pb.ptr_ = nullptr;
            }

            ~PageBoundary() {
                if(!!ptr_) {
                    int ret = ::munmap(ptr_, 2*PAGE_SIZE);
                    if(ret < 0) {
                        perror("PageBoundary::munmap");
                    }
                }
            }

            void* get(size_t readableBytes) {
                assert(readableBytes < PAGE_SIZE);
                return boundary() - readableBytes;
            }

            size_t availableSize() const {
                return PAGE_SIZE;
            }

            [[nodiscard]] bool tryLock() {
                if(::mprotect(boundary(), PAGE_SIZE, PROT_NONE) < 0) {
                    perror("lock");
                    return false;
                }
                return true;
            }

            [[nodiscard]] bool tryUnlock() {
                if(::mprotect(boundary(), PAGE_SIZE, PROT_READ|PROT_WRITE) < 0) {
                    perror("unlock");
                    return false;
                }
                return true;
            }

        private:
            PageBoundary(char* ptr) : ptr_(ptr) { }
            char* boundary() { return ptr_ + PAGE_SIZE; }
            char* ptr_ { nullptr };
            const size_t PAGE_SIZE = 0x1000;
        };


        std::optional<PageBoundary> boundary = PageBoundary::tryCreate();
        if(!boundary) return {};
        if(size > boundary->availableSize()) return {};

        for(size_t s = 1; s <= size; ++s) {
            void* ptr = boundary->get(s);
            ::memcpy(ptr, data, s);
            if(!boundary->tryLock()) return {};
            int ret = ::ioctl(fd.fd, request, ptr);
            if(ret < 0) {
                if(errno != EFAULT) return -errno;
                if(!boundary->tryUnlock()) return {};
                continue;
            } else {
                return (ssize_t)s;
            }
        }
        return {};
    }

    std::optional<size_t> Host::ioctlRequiredBufferSize(unsigned long request) {
        switch(request) {
            case TCGETS:
            case TCSETS: return sizeof(termios);
            case FIOCLEX:
            case FIONCLEX: return 0;
            case TIOCGWINSZ: 
            case TIOCSWINSZ: return sizeof(winsize);
            case TCSETSW: return sizeof(termios);
            case TIOCGPGRP: return sizeof(pid_t);
            default: break;
        }
        return {};
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

    int Host::getpid() {
        return (int)::getpid();
    }

    int Host::getppid() {
        return (int)::getppid();
    }

    ErrnoOrBuffer Host::getgroups(int size) {
        std::vector<gid_t> groups((size_t)size, 0);
        int ret = ::getgroups(size, groups.data());
        if(ret < 0) return ErrnoOrBuffer(-errno);
        return ErrnoOrBuffer(Buffer{std::move(groups)});
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

    Host::UserCredentials Host::getUserCredentials() {
        uid_t ruid { 0 };
        uid_t euid { 0 };
        uid_t suid { 0 };
        int ret = ::getresuid(&ruid, &euid, &suid);
        (void)ret;
        assert(ret == 0);
        gid_t rgid { 0 };
        gid_t egid { 0 };
        gid_t sgid { 0 };
        ret = ::getresgid(&rgid, &egid, &sgid);
        (void)ret;
        assert(ret == 0);
        return UserCredentials {
            (int)ruid,
            (int)euid,
            (int)suid,
            (int)rgid,
            (int)egid,
            (int)sgid,
        };
    }

    ErrnoOrBuffer Host::getcwd(size_t size) {
        std::vector<u8> path;
        path.resize(size, 0x0);
        char* cwd = ::getcwd((char*)path.data(), path.size());
        if(!cwd) return ErrnoOrBuffer(-errno);
        size_t actualSize = 0;
        for(auto it = path.begin(); it != path.end() && *it != '\0'; ++it) ++actualSize;
        path.resize(actualSize+1);
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

    Buffer Host::clock_gettime(PreciseTime time) {
        struct timespec ts;
        assert(time.seconds <= std::numeric_limits<time_t>::max());
        assert(time.nanoseconds <= std::numeric_limits<long>::max());
        ts.tv_sec = (time_t)time.seconds;
        ts.tv_nsec = (long)time.nanoseconds;
        return Buffer(ts);
    }

    Buffer Host::clock_getres() {
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1;
        return Buffer(ts);
    }

    Buffer Host::gettimeofday(PreciseTime time) {
        struct timeval tv;
        assert(time.seconds <= std::numeric_limits<time_t>::max());
        assert(time.nanoseconds / 1'000 <= std::numeric_limits<suseconds_t>::max());
        tv.tv_sec = (time_t)time.seconds;
        tv.tv_usec = (suseconds_t)(time.nanoseconds / 1'000);
        return Buffer(tv);
    }

    Buffer Host::gettimezone() {
        struct timezone tz;
        tz.tz_minuteswest = 0;    // minutes west of Greenwich
        tz.tz_dsttime = 0;        // type of DST correction. 0 should be none
        return Buffer(tz);
    }

    ErrnoOrBuffer Host::getrlimit([[maybe_unused]] pid_t pid, int resource) {
        switch(resource) {
            case RLIMIT_STACK: {
                struct rlimit stackLimit {
                    static_cast<rlim_t>(64*0x1000),
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

    size_t Host::selectFdSetSize() {
        return FD_SETSIZE;
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
            default: break;
        }
        return {};
    }

}