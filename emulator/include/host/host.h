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

namespace kernel::gnulinux {

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

        struct ArchPrctl {
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
            enum Command {
                DUPFD,
                DUPFD_CLOEXEC,
                GETFD,
                SETFD,
                GETFL,
                SETFL,
                UNSUPPORTED,
            };

            static Command toCommand(int cmd);
            static int fdCloExec();
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
            static bool isSemaphore(int flags);
        };

        struct EpollFlags {
            static bool isCloseOnExec(int flags);
            static bool isOther(int flags);
        };

        struct EpollCtlOp {
            static bool isAdd(int op);
            static bool isMod(int op);
            static bool isDel(int op);
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

        struct Statx {
            static bool isAtEmptyPath(int flags);
            static bool wantStatxType(unsigned int mask);
            static bool wantStatxMode(unsigned int mask);
            static bool wantStatxNlink(unsigned int mask);
            static bool wantStatxUid(unsigned int mask);
            static bool wantStatxGid(unsigned int mask);
            static bool wantStatxAtime(unsigned int mask);
            static bool wantStatxMtime(unsigned int mask);
            static bool wantStatxCtime(unsigned int mask);
            static bool wantStatxIno(unsigned int mask);
            static bool wantStatxSize(unsigned int mask);
            static bool wantStatxBlocks(unsigned int mask);
            static bool wantStatxBasicStat(unsigned int mask);
            static bool wantStatxBtime(unsigned int mask);
            static bool wantStatxAll(unsigned int mask);
        };

        class StatxBuilder {
        public:
            Buffer create();

            StatxBuilder& addBlocksize(u32 value) { blocksize_ = value; return *this; }
            StatxBuilder& addAttributes(u64 value) { attributes_ = value; return *this; }
            StatxBuilder& addNlink(u32 value) { nlink_ = value; return *this; }
            StatxBuilder& addUid(u32 value) { uid_ = value; return *this; }
            StatxBuilder& addGid(u32 value) { gid_ = value; return *this; }
            StatxBuilder& addMode(u16 value) { mode_ = value; return *this; }
            StatxBuilder& addIno(u64 value) { ino_ = value; return *this; }
            StatxBuilder& addSize(u64 value) { size_ = value; return *this; }
            StatxBuilder& addBlocks(u64 value) { blocks_ = value; return *this; }
            StatxBuilder& addAttributesMask(u64 value) { attributesMask_ = value; return *this; }
            StatxBuilder& addRdevMajor(u32 value) { rdevMajor_ = value; return *this; }
            StatxBuilder& addRdevMinor(u32 value) { rdevMinor_ = value; return *this; }
            StatxBuilder& addDevMajor(u32 value) { devMajor_ = value; return *this; }
            StatxBuilder& addDevMinor(u32 value) { devMinor_ = value; return *this; }

        private:
            std::optional<u32> blocksize_;
            std::optional<u64> attributes_;
            std::optional<u32> nlink_;
            std::optional<u32> uid_;
            std::optional<u32> gid_;
            std::optional<u16> mode_;
            std::optional<u64> ino_;
            std::optional<u64> size_;
            std::optional<u64> blocks_;
            std::optional<u64> attributesMask_;
            std::optional<u32> rdevMajor_;
            std::optional<u32> rdevMinor_;
            std::optional<u32> devMajor_;
            std::optional<u32> devMinor_;
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

        struct Ioctl {
            static bool isFIOCLEX(unsigned long request);
            static bool isFIONCLEX(unsigned long request);
            static bool isTCGETS(unsigned long request);
            static bool isTCSETS(unsigned long request);
            static bool isTCSETSW(unsigned long request);
            static bool isFIONBIO(unsigned long request);
            static bool isTIOCGWINSZ(unsigned long request);
            static bool isTIOCSWINSZ(unsigned long request);
            static bool isTIOCGPGRP(unsigned long request);
        };

        struct Prctl {
            static bool isSetName(int option);
            static bool isCapabilitySetRead(int option);
        };

        struct Lock {
            static bool isShared(int operation);
            static bool isExclusive(int operation);
            static bool isUnlock(int operation);
            static bool isNonBlocking(int operation);
        };

        struct Fstatat {
            static bool isEmptyPath(int flags);
            static bool isNoAutomount(int flags);
            static bool isSymlinkNofollow(int flags);
        };

        struct ShmGet {
            static bool isIpcPrivate(key_t key);

            static int getModePermissions(int shmflg);
            static bool isIpcCreate(int shmflg);
            static bool isIpcExcl(int shmflg);
        };

        struct ShmAt {
            static bool isReadOnly(int shmflg);
            static bool isExecute(int shmflg);
            static bool isRemap(int shmflg);
        };

        struct ShmCtl {
            static bool isStat(int cmd);
            static bool isSet(int cmd);
            static bool isRmid(int cmd);
        };

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