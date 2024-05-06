#ifndef HOST_H
#define HOST_H

#include "utils/buffer.h"
#include "utils/erroror.h"
#include "utils/utils.h"
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kernel {

    template<typename T>
    struct BufferAndReturnValue {
        Buffer buffer;
        T returnValue;
    };

    class Host {
    public:
        Host();
        ~Host();

        // math
        static f80 round(f80);

        template<typename U>
        struct IdivResult {
            U quotient;
            U remainder;
        };

        static IdivResult<u32> idiv32(u32 upperDividend, u32 lowerDividend, u32 divisor);
        static IdivResult<u64> idiv64(u64 upperDividend, u64 lowerDividend, u64 divisor);

        // cpu
        struct CPUID {
            u32 a, b, c, d;
        };
        static CPUID cpuid(u32 a, u32 c);

        struct XGETBV {
            u32 a, d;
        };
        static XGETBV xgetbv(u32 c);

        struct Mmap {
            static bool isAnonymous(int flags);
            static bool isFixed(int flags);
        };

        struct Prctl {
            static bool isSetFS(int code);
        };

        struct Iovec {
            Buffer iov;
        };

        struct Msghdr {
            Buffer msg_name;
            std::vector<Iovec> msg_iov;
            Buffer msg_control;
            int msg_flags;
        };

        // syscalls
        struct FD {
            int fd;
        };

        struct Open {
            static bool isReadable(int flag);
            static bool isWritable(int flag);
            static bool isAppending(int flag);
            static bool isTruncating(int flag);
            static bool isCreatable(int flag);
            static bool isCloseOnExec(int flag);
        };

        struct Mode {
            static bool isUserReadable(unsigned int mode);
            static bool isUserWritable(unsigned int mode);
            static bool isUserExecutable(unsigned int mode);
        };

        static FD cwdfd();

        static ErrnoOrBuffer read(FD, size_t count);
        static ErrnoOrBuffer pread64(FD, size_t count, off_t offset);
        static ssize_t write(FD, const u8* data, size_t count);
        static ssize_t pwrite64(FD, const u8* data, size_t count, off_t offset);
        int close(FD);
        FD dup(FD);

        static size_t iovecRequiredBufferSize();
        static size_t iovecLen(const Buffer& buffer, size_t i);
        static u64 iovecBase(const Buffer& buffer, size_t i);
        static ssize_t writev(FD, const std::vector<Buffer>& buffer);

        static ErrnoOrBuffer stat(const std::string& path);
        static ErrnoOrBuffer fstat(FD fd);
        static ErrnoOrBuffer lstat(const std::string& path);
        static ErrnoOrBuffer fstatat64(FD dirfd, const std::string& path, int flags);
        static off_t lseek(FD fd, off_t offset, int whence);
        FD openat(FD dirfd, const std::string& pathname, int flags, mode_t mode);
        static int access(const std::string& path, int mode);

        static ErrnoOrBuffer statfs(const std::string& path);
        static ErrnoOrBuffer statx(FD dirfd, const std::string& path, int flags, unsigned int mask);

        static ErrnoOrBuffer getxattr(const std::string& path, const std::string& name, size_t size);
        static ErrnoOrBuffer lgetxattr(const std::string& path, const std::string& name, size_t size);

        static std::string fcntlName(int cmd);
        static int fcntl(FD fd, int cmd, int arg);

        static FD socket(int domain, int type, int protocol);
        static int connect(int sockfd, const Buffer& addr);
        static ErrnoOrBuffer getsockname(int sockfd, u32 buffersize);
        static ErrnoOrBuffer getpeername(int sockfd, u32 buffersize);
        static int bind(FD sockfd, const Buffer& addr);
        static int shutdown(FD sockfd, int how);

        static ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress);
        static ssize_t recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags);
        static ssize_t sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags);

        static ErrnoOrBuffer readlink(const std::string& path, size_t count);
        static ErrnoOrBuffer uname();

        static std::string ioctlName(unsigned long request);
        static size_t ioctlRequiredBufferSize(unsigned long request);
        static ErrnoOrBuffer ioctl(FD fd, unsigned long request, const Buffer& buffer);

        static ErrnoOrBuffer sysinfo();
        static int getuid();
        static int getgid();
        static int geteuid();
        static int getegid();
        static int getpgrp();
        
        static ErrnoOrBuffer getcwd(size_t size);
        static ErrnoOrBuffer getdents64(FD fd, size_t count);
        static int chdir(const std::string& path);

        static ErrnoOrBuffer clock_gettime(clockid_t clockid);
        static time_t time();
        static ErrnoOr<std::pair<Buffer, Buffer>> gettimeofday();

        static std::vector<u8> readFromFile(FD fd, size_t length, off_t offset);

        static ErrnoOrBuffer getrlimit(pid_t pid, int resource);

        static size_t pollRequiredBufferSize(size_t nfds);
        static ErrnoOr<BufferAndReturnValue<int>> poll(const Buffer&, u64 nfds, int timeout);
        static FD epoll_create1(int flags);

        static FD eventfd2(unsigned int initval, int flags);

        static int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval* timeout);
        static int pselect6(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timespec* timeout, const sigset_t* sigmask);

        enum class AUX_TYPE {
            NIL,
            ENTRYPOINT,
            PROGRAM_HEADERS,
            PROGRAM_HEADER_ENTRY_SIZE,
            PROGRAM_HEADER_COUNT,
            RANDOM_VALUE_ADDRESS,
            PLATFORM_STRING_ADDRESS,
            VDSO_ADDRESS,
            EXEC_FILE_DESCRIPTOR,
            EXEC_PATH_NAME,
            UID,
            GID,
            EUID,
            EGID,
            SECURE,
        };

        struct AuxVal {
            u64 type;
            u64 value;
        };

        static std::optional<AuxVal> getauxval(AUX_TYPE type);
        
        std::optional<std::string> filename(FD fd) const;

    private:
        std::unordered_map<int, std::string> openFiles_;
    };
}

#endif