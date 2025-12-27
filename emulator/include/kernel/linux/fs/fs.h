#ifndef FS_H
#define FS_H

#include "kernel/linux/fs/fsflags.h"
#include "kernel/linux/fs/ioctl.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include "bitflags.h"
#include "span.h"
#include <bitset>
#include <deque>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace kernel::gnulinux {

    class Directory;
    class File;
    class OpenFileDescription;
    class Path;
    class Pipe;
    class Tty;
    class ProcFS;
    class Symlink;

    struct FD {
        int fd;

        bool operator==(FD other) const { return fd == other.fd; }
        bool operator!=(FD other) const { return fd != other.fd; }
    };

    struct FileDescriptor {
        FileDescriptor(OpenFileDescription* ofd, bool closeOnExec) :
                openFiledescription(ofd), closeOnExec(closeOnExec) { }

        OpenFileDescription* openFiledescription { nullptr };
        bool closeOnExec { false };
    };

    class FS {
    public:
        FS();
        ~FS();

        void resetProcFS(int pid, const Path& programFilePath);

        static BitFlags<AccessMode> toAccessMode(int flags);
        static BitFlags<CreationFlags> toCreationFlags(int flags);
        static BitFlags<StatusFlags> toStatusFlags(int flags);
        static Permissions fromMode(unsigned int mode);

        Directory* root() { return root_.get(); }
        Directory* cwd() { return currentWorkDirectory_; }

        std::optional<Path> resolvePath(const Directory* base, const std::string& pathname) const;
        std::optional<Path> resolvePath(FD dirfd, const Directory* base, const std::string& pathname) const;

        enum AllowEmptyPathname {
            NO,
            YES,
        };
        std::optional<Path> resolvePath(FD dirfd, const Directory* base, const std::string& pathname, AllowEmptyPathname tag) const;

        FD open(const Path& path,
                BitFlags<AccessMode> accessMode,
                BitFlags<CreationFlags> creationFlags,
                BitFlags<StatusFlags> statusFlags,
                Permissions permissions);
        int close(FD fd);

        FD dup(FD fd);
        FD dup2(FD oldfd, FD newfd);
        FD dup3(FD oldfd, FD newfd, int flags);

        int mkdir(const Path& path);
        int rename(const Path& oldpath, const Path& newpath);
        int unlink(const Path& path);

        ErrnoOrBuffer readlink(const Path& path, size_t bufferSize);

        int access(const Path& path, int mode) const;

        FD memfd_create(const std::string& name, unsigned int flags);

        ErrnoOrBuffer read(FD fd, size_t count);
        ErrnoOrBuffer pread(FD fd, size_t count, off_t offset);
        ssize_t readv(FD fd, std::vector<Buffer>* buffers);

        ssize_t write(FD fd, const u8* buf, size_t count);
        ssize_t pwrite(FD fd, const u8* buf, size_t count, off_t offset);
        ssize_t writev(FD fd, const std::vector<Buffer>& buffers);

        ErrnoOrBuffer stat(const Path& path);
        ErrnoOrBuffer fstat(FD fd);
        ErrnoOrBuffer statx(const Path& path, int flags, unsigned int mask);
        ErrnoOrBuffer fstatat64(const Path& path, int flags);

        ErrnoOrBuffer fstatfs(FD fd);

        off_t lseek(FD fd, off_t offset, int whence);

        ErrnoOrBuffer getdents64(FD fd, size_t count);
        int fcntl(FD fd, int cmd, int arg);

        ErrnoOrBuffer ioctl(FD fd, Ioctl request, const Buffer& buffer);

        int flock(FD fd, int operation);

        int fallocate(FD fd, int mode, off_t offset, off_t len);
        int truncate(const Path& path, off_t length);
        int ftruncate(FD fd, off_t length);

        struct EpollEvent {
            BitFlags<EpollEventType> events;
            u64 data;
        };

        FD eventfd2(unsigned int initval, int flags);
        FD epoll_create1(int flags);
        int epoll_ctl(FD epfd, int op, FD fd, BitFlags<EpollEventType> events, u64 data);
        int epollWaitImmediate(FD epfd, std::vector<EpollEvent>* events);
        void doEpollWait(FD epfd, std::vector<EpollEvent>* events);

        FD socket(int domain, int type, int protocol);
        int connect(FD sockfd, const Buffer& buffer);
        int bind(FD sockfd, const Buffer& name);
        int shutdown(FD sockfd, int how);
        ErrnoOrBuffer getpeername(FD sockfd, u32 buffersize);
        ErrnoOrBuffer getsockname(FD sockfd, u32 buffersize);
        ErrnoOrBuffer getsockopt(FD sockfd, int level, int optname, const Buffer& buffer);
        int setsockopt(FD sockfd, int level, int optname, const Buffer& buffer);

        struct Message {
            Buffer msg_name;
            std::vector<Buffer> msg_iov;
            Buffer msg_control;
            int msg_flags;
        };

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress);
        ssize_t send(FD sockfd, const Buffer& buffer, int flags);
        ssize_t recvmsg(FD sockfd, int flags, Message* message);
        ssize_t sendmsg(FD sockfd, int flags, const Message& message);

        enum class PollEvent : i16 {
            NONE = 0x0,
            CAN_READ = 0x1,
            CAN_WRITE = 0x4,
            INVALID_REQUEST = 0x20,
        };

        friend PollEvent operator&(PollEvent a, PollEvent b) { return (PollEvent)((i16)a & (i16)b); }
        friend PollEvent operator|(PollEvent a, PollEvent b) { return (PollEvent)((i16)a | (i16)b); }
        friend PollEvent operator~(PollEvent a) { return (PollEvent)(~(i16)a); }

        struct PollData {
            i32 fd;
            PollEvent events;
            PollEvent revents;
        };
        
        ErrnoOr<BufferAndReturnValue<int>> pollImmediate(const std::vector<PollData>& pfds);
        void doPoll(std::vector<PollData>* data);

        struct SelectData {
            i32 nfds;
            std::bitset<FD_SETSIZE> readfds;
            std::bitset<FD_SETSIZE> writefds;
            std::bitset<FD_SETSIZE> exceptfds;
        };

        int selectImmediate(SelectData* selectData);

        ErrnoOr<std::pair<FD, FD>> pipe2(int flags);

        std::string filename(FD fd);
        void dumpSummary() const;

    private:
        enum class FollowSymlink {
            NO,
            YES,
        };

        Directory* ensurePathExceptLast(const Path& path);
        Directory* ensureCompletePath(const Path& path);

        File* tryGetFile(const Path& path, FollowSymlink);
        std::unique_ptr<File> tryTakeFile(const Path& path);
        Directory* ensurePathImpl(Span<const std::string> components);

        File* resolveSymlink(const Symlink&, u32 maxLinks = 0);

        void findCurrentWorkDirectory();
        void createStandardStreams(const Path& ttypath);
        FD insertNode(std::unique_ptr<File> file, BitFlags<AccessMode>, BitFlags<StatusFlags>, bool closeOnExec);
        FD allocateFd();

        void removeFromOrphans(File*);
        void removeClosedPipes();

        FileDescriptor* findFileDescriptor(FD fd);
        OpenFileDescription* findOpenFileDescription(FD fd);

        void checkFileRefCount(File* file) const;
        void checkFileDescriptions() const;

        static int assembleAccessModeAndFileStatusFlags(BitFlags<AccessMode>, BitFlags<StatusFlags>);

        std::unique_ptr<Directory> root_;
        Tty* tty_ { nullptr };
        ProcFS* procfs_ { nullptr };
        std::vector<std::unique_ptr<File>> orphanFiles_;
        std::vector<std::unique_ptr<Pipe>> pipes_;
        Directory* currentWorkDirectory_ { nullptr };
        std::vector<std::unique_ptr<FileDescriptor>> fileDescriptors_;
        std::vector<std::unique_ptr<OpenFileDescription>> openFileDescriptions_;
    };

}

#endif