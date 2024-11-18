#ifndef HOST_H
#define HOST_H

#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include "kernel/timers.h"
#include "utils.h"
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kernel {

    class Host {
    public:
        Host();
        ~Host();

        struct Mmap {
            static bool isAnonymous(int flags);
            static bool isFixed(int flags);
            static bool isPrivate(int flags);
            static bool isShared(int flags);
        };

        struct Madvise {
            static bool isDontNeed(int advice);
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
            static bool isCloseOnExec(int flag);
            static bool isCreatable(int flag);
            static bool isDirectory(int flag);
            static bool isLargeFile(int flag);
            static bool isNonBlock(int flag);
            static bool isTruncating(int flag);
        };

        struct Fcntl {
            static bool isGetFd(int cmd);
            static bool isSetFd(int cmd);
            static bool isGetFl(int cmd);
            static bool isSetFl(int cmd);
        };

        struct Mode {
            static bool isUserReadable(unsigned int mode);
            static bool isUserWritable(unsigned int mode);
            static bool isUserExecutable(unsigned int mode);
        };

        struct FallocateMode {
            static bool isKeepSize(int mode);
            static bool isPunchHole(int mode);
            static bool isNoHidestale(int mode);
            static bool isCollapseRange(int mode);
            static bool isZeroRange(int mode);
            static bool isInsertRange(int mode);
            static bool isUnshareRange(int mode);
        };

        struct MemfdFlags {
            static bool isCloseOnExec(unsigned int flags);
            static bool allowsSealing(unsigned int flags);
            static bool isOther(unsigned int flags);
        };

        struct Eventfd2Flags {
            static bool isCloseOnExec(int flags);
            static bool isNonBlock(int flags);
            static bool isOther(int flags);
        };

        struct EpollFlags {
            static bool isCloseOnExec(int flags);
            static bool isOther(int flags);
        };

        struct SocketType {
            static bool isCloseOnExec(int type);
            static bool isNonBlock(int type);
        };

        struct Lseek {
            static bool isSeekSet(int whence);
            static bool isSeekCur(int whence);
            static bool isSeekEnd(int whence);
        };

        struct SchedAttr {
            u32 size;
            u32 schedPolicy;
            u64 schedFlags;
            i32 schedNice;
            u32 schedPriority;
            u64 schedRuntime;
            u64 schedDeadline;
            u64 schedPeriod;
        };

        static SchedAttr getSchedulerAttributes();

        struct CloneFlags {
            bool childClearTid;
            bool childSetTid;
            bool clearSignalHandlers;
            bool cloneSignalHandlers;
            bool cloneFiles;
            bool cloneFs;
            bool cloneIo;
            bool cloneParent;
            bool parentSetTid;
            bool clonePidFd;
            bool setTls;
            bool cloneThread;
            bool cloneVm;
            bool cloneVfork;
        };

        static CloneFlags fromCloneFlags(unsigned long flags);

        static FD cwdfd();

        static size_t iovecRequiredBufferSize();
        static size_t iovecLen(const Buffer& buffer, size_t i);
        static u64 iovecBase(const Buffer& buffer, size_t i);

        static ErrnoOrBuffer stat(const std::string& path);
        static ErrnoOrBuffer fstat(FD fd);
        static ErrnoOrBuffer lstat(const std::string& path);
        static ErrnoOrBuffer fstatat64(FD dirfd, const std::string& path, int flags);
        static int access(const std::string& path, int mode);

        static ErrnoOrBuffer statfs(const std::string& path);
        static ErrnoOrBuffer statx(FD dirfd, const std::string& path, int flags, unsigned int mask);

        static ErrnoOrBuffer getxattr(const std::string& path, const std::string& name, size_t size);
        static ErrnoOrBuffer lgetxattr(const std::string& path, const std::string& name, size_t size);

        static std::string fcntlName(int cmd);
        static int fcntl(FD fd, int cmd, int arg);

        static ErrnoOrBuffer readlink(const std::string& path, size_t count);
        static ErrnoOrBuffer uname();

        static std::string ioctlName(unsigned long request);
        static std::optional<size_t> ioctlRequiredBufferSize(unsigned long request);
        static std::optional<ssize_t> tryGuessIoctlBufferSize(FD fd, unsigned long request, const u8* data, size_t size);

        static ErrnoOrBuffer sysinfo();
        static int getuid();
        static int getgid();
        static int geteuid();
        static int getegid();
        static int getpgrp();
        static int getpid();
        static int getppid();
        static ErrnoOrBuffer getgroups(int size);

        struct UserCredentials {
            int ruid;
            int euid;
            int suid;
            int rgid;
            int egid;
            int sgid;
        };

        static UserCredentials getUserCredentials();
        
        static ErrnoOrBuffer getcwd(size_t size);
        static ErrnoOrBuffer getdents64(FD fd, size_t count);
        static int chdir(const std::string& path);

        static Buffer clock_gettime(PreciseTime time);
        static Buffer clock_getres();
        static Buffer gettimeofday(PreciseTime time);
        static Buffer gettimezone();

        static ErrnoOrBuffer getrlimit(pid_t pid, int resource);

        static size_t pollRequiredBufferSize(size_t nfds);
        static size_t selectFdSetSize();

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
    };
}

#endif