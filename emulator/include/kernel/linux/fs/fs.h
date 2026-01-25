#ifndef FS_H
#define FS_H

#include "kernel/linux/fs/fsflags.h"
#include "kernel/linux/fs/ioctl.h"
#include "kernel/linux/fs/path.h"
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
        std::shared_ptr<OpenFileDescription> openFiledescription;
        bool closeOnExec { false };
    };

    class FS;

    struct CurrentDirectoryOrDirectoryDescriptor {
        bool isCurrentDirectory { true };
        Directory* cwd { nullptr };
        FileDescriptor directoryDescriptor;
    };

    class FileDescriptors {
    public:
        explicit FileDescriptors(FS& fs) : fs_(fs) { }
        void createStandardStreams(const Path& ttypath);

        std::unique_ptr<FileDescriptors> clone();

        FileDescriptor operator[](FD fd) {
            auto* descriptor = findFileDescriptor(fd);
            if(!descriptor) return {};
            return *descriptor;
        }

        FileDescriptor operator[](int fd) {
            auto* descriptor = findFileDescriptor(FD{fd});
            if(!descriptor) return {};
            return *descriptor;
        }

        CurrentDirectoryOrDirectoryDescriptor dirfd(FD dirfd, Directory* cwd);

        FD open(const Path& path,
                BitFlags<AccessMode> accessMode,
                BitFlags<CreationFlags> creationFlags,
                BitFlags<StatusFlags> statusFlags,
                Permissions permissions);

        int close(FD);

        int fcntl(FD fd, int cmd, int arg);

        FD dup(FD fd);
        FD dup2(FD oldfd, FD newfd);
        FD dup3(FD oldfd, FD newfd, int flags);

        FD memfd_create(const std::string& name, unsigned int flags);

        FD eventfd2(unsigned int initval, int flags);
        FD epoll_create1(int flags);
        FD socket(int domain, int type, int protocol);
        ErrnoOr<std::pair<FD, FD>> pipe2(int flags);

        void dumpSummary() const;

    private:
        FD allocateFd();
        FileDescriptor* findFileDescriptor(FD fd);
        std::shared_ptr<OpenFileDescription> findOpenFileDescription(FD fd);

        FS& fs_;
        std::vector<std::unique_ptr<FileDescriptor>> fileDescriptors_;
    };

    class FS {
    public:
        FS();
        ~FS();

        Directory* findCurrentWorkDirectory(const Path& path);
        Path ttyPath() const;

        void resetProcFS(int pid, const Path& programFilePath);

        static BitFlags<AccessMode> toAccessMode(int flags);
        static BitFlags<CreationFlags> toCreationFlags(int flags);
        static BitFlags<StatusFlags> toStatusFlags(int flags);
        static Permissions fromMode(unsigned int mode);

        Directory* root() { return root_.get(); }

        std::optional<Path> resolvePath(const Directory* cwd, const std::string& pathname) const;
        std::optional<Path> resolvePath(CurrentDirectoryOrDirectoryDescriptor dirfd, const std::string& pathname) const;

        enum AllowEmptyPathname {
            NO,
            YES,
        };
        std::optional<Path> resolvePath(CurrentDirectoryOrDirectoryDescriptor dirfd, const std::string& pathname, AllowEmptyPathname tag) const;

        ErrnoOr<FileDescriptor> open(const Path& path,
                BitFlags<AccessMode> accessMode,
                BitFlags<CreationFlags> creationFlags,
                BitFlags<StatusFlags> statusFlags,
                Permissions permissions);
        int close(FileDescriptor fd);

        int mkdir(const Path& path);
        int rename(const Path& oldpath, const Path& newpath);
        int unlink(const Path& path);

        ErrnoOrBuffer readlink(const Path& path, size_t bufferSize);

        int access(const Path& path, int mode) const;

        ErrnoOr<FileDescriptor> memfd_create(const std::string& name, unsigned int flags);

        ErrnoOrBuffer read(FileDescriptor fd, size_t count);
        ErrnoOrBuffer pread(FileDescriptor fd, size_t count, off_t offset);
        ssize_t readv(FileDescriptor fd, std::vector<Buffer>* buffers);

        ssize_t write(FileDescriptor fd, const u8* buf, size_t count);
        ssize_t pwrite(FileDescriptor fd, const u8* buf, size_t count, off_t offset);
        ssize_t writev(FileDescriptor fd, const std::vector<Buffer>& buffers);

        ErrnoOrBuffer stat(const Path& path);
        ErrnoOrBuffer fstat(FileDescriptor fd);
        ErrnoOrBuffer statx(const Path& path, int flags, unsigned int mask);
        ErrnoOrBuffer fstatat64(const Path& path, int flags);

        ErrnoOrBuffer fstatfs(FileDescriptor fd);

        off_t lseek(FileDescriptor fd, off_t offset, int whence);

        ErrnoOrBuffer getdents64(FileDescriptor fd, size_t count);
        int fcntl(FileDescriptor& fd, int cmd, int arg);

        ErrnoOrBuffer ioctl(FileDescriptor fd, Ioctl request, const Buffer& buffer);

        int flock(FileDescriptor fd, int operation);

        int fallocate(FileDescriptor fd, int mode, off_t offset, off_t len);
        int truncate(const Path& path, off_t length);
        int ftruncate(FileDescriptor fd, off_t length);

        struct EpollEvent {
            BitFlags<EpollEventType> events;
            u64 data;
        };

        ErrnoOr<FileDescriptor> eventfd2(unsigned int initval, int flags);
        ErrnoOr<FileDescriptor> epoll_create1(int flags);
        int epoll_ctl(FileDescriptor epfd, int op, FileDescriptor fd, BitFlags<EpollEventType> events, u64 data);
        int epollWaitImmediate(FileDescriptor epfd, std::vector<EpollEvent>* events);
        void doEpollWait(FileDescriptor epfd, std::vector<EpollEvent>* events);

        ErrnoOr<FileDescriptor> socket(int domain, int type, int protocol);
        int connect(FileDescriptor sockfd, const Buffer& buffer);
        int bind(FileDescriptor sockfd, const Buffer& name);
        int shutdown(FileDescriptor sockfd, int how);
        ErrnoOrBuffer getpeername(FileDescriptor sockfd, u32 buffersize);
        ErrnoOrBuffer getsockname(FileDescriptor sockfd, u32 buffersize);
        ErrnoOrBuffer getsockopt(FileDescriptor sockfd, int level, int optname, const Buffer& buffer);
        int setsockopt(FileDescriptor sockfd, int level, int optname, const Buffer& buffer);

        struct Message {
            Buffer msg_name;
            std::vector<Buffer> msg_iov;
            Buffer msg_control;
            int msg_flags;
        };

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(FileDescriptor sockfd, size_t len, int flags, bool requireSrcAddress);
        ssize_t send(FileDescriptor sockfd, const Buffer& buffer, int flags);
        ssize_t recvmsg(FileDescriptor sockfd, int flags, Message* message);
        ssize_t sendmsg(FileDescriptor sockfd, int flags, const Message& message);

        struct PollFd {
            i32 fd;
            PollEvent events;
            PollEvent revents;
        };

        struct PollData {
            i32 fd;
            FileDescriptor descriptor;
            PollEvent events;
            PollEvent revents;
        };
        
        ErrnoOr<BufferAndReturnValue<int>> pollImmediate(const std::vector<PollData>& pfds);
        void doPoll(std::vector<PollData>* data);

        struct SelectData {
            std::vector<FileDescriptor> fds;
            std::bitset<FD_SETSIZE> readfds;
            std::bitset<FD_SETSIZE> writefds;
            std::bitset<FD_SETSIZE> exceptfds;
        };

        int selectImmediate(SelectData* selectData);

        ErrnoOr<std::pair<FileDescriptor, FileDescriptor>> pipe2(int flags);

        std::string filename(FileDescriptor fd);
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

        FileDescriptor insertNode(std::unique_ptr<File> file, BitFlags<AccessMode>, BitFlags<StatusFlags>, bool closeOnExec);

        void removeFromOrphans(File*);
        void removeClosedPipes();

        void checkFileRefCount(File* file) const;

        static int assembleAccessModeAndFileStatusFlags(BitFlags<AccessMode>, BitFlags<StatusFlags>);

        std::unique_ptr<Directory> root_;
        Tty* tty_ { nullptr };
        ProcFS* procfs_ { nullptr };
        std::vector<std::unique_ptr<File>> orphanFiles_;
        std::vector<std::unique_ptr<Pipe>> pipes_;
    };

}

#endif