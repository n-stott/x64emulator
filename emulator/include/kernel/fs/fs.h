#ifndef FS_H
#define FS_H

#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include "span.h"
#include <bitset>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    class Directory;
    class File;
    class Kernel;
    class OpenFileDescription;
    class Path;
    class Pipe;
    class TtyDevice;

    class FS {
    public:
        explicit FS(Kernel& kernel);
        ~FS();

        struct OpenFlags {
            bool read { false };
            bool write { false };
            bool append { false };
            bool truncate { false };
            bool create { false };
            bool closeOnExec { false };
            bool directory { false };
        };

        struct Permissions {
            bool userReadable { false };
            bool userWriteable { false };
            bool userExecutable { false };
        };

        static OpenFlags fromFlags(int flags);
        static Permissions fromMode(unsigned int mode);

        struct FD {
            int fd;

            bool operator==(FD other) const { return fd == other.fd; }
            bool operator!=(FD other) const { return fd != other.fd; }
        };

        Kernel& kernel() { return kernel_; }

        Directory* root() { return root_.get(); }
        Directory* cwd() { return currentWorkDirectory_; }

        std::string toAbsolutePathname(const std::string& pathname) const;
        std::string toAbsolutePathname(const std::string& pathname, FD dirfd) const;
        Directory* ensurePathExceptLast(const Path& path);
        Directory* ensureCompletePath(const Path& path);

        FD open(FD dirfd, const std::string& pathname, OpenFlags flags, Permissions permissions);
        FD dup(FD fd);
        FD dup2(FD oldfd, FD newfd);
        int close(FD fd);

        int mkdir(const std::string& pathname);
        int rename(const std::string& oldname, const std::string& newname);
        int unlink(const std::string& pathname);

        int access(const std::string& pathname, int mode);
        int faccessat(FD dirfd, const std::string& pathname, int mode);

        FD memfd_create(const std::string& name, unsigned int flags);

        ErrnoOrBuffer read(FD fd, size_t count);
        ErrnoOrBuffer pread(FD fd, size_t count, off_t offset);

        ssize_t write(FD fd, const u8* buf, size_t count);
        ssize_t pwrite(FD fd, const u8* buf, size_t count, off_t offset);
        ssize_t writev(FD fd, const std::vector<Buffer>& buffers);

        ErrnoOrBuffer stat(const std::string& pathname);
        ErrnoOrBuffer fstat(FD fd);
        ErrnoOrBuffer statx(FD dirfd, const std::string& pathname, int flags, unsigned int mask);
        ErrnoOrBuffer fstatat64(FD dirfd, const std::string& pathname, int flags);

        ErrnoOrBuffer fstatfs(FD fd);

        off_t lseek(FD fd, off_t offset, int whence);

        ErrnoOrBuffer getdents64(FD fd, size_t count);
        int fcntl(FD fd, int cmd, int arg);
        ErrnoOrBuffer ioctl(FD fd, unsigned long request, const Buffer& buffer);
        ErrnoOrBuffer ioctlWithBufferSizeGuess(FD fd, unsigned long request, const Buffer& buffer);

        int flock(FD fd, int operation);

        int fallocate(FD fd, int mode, off_t offset, off_t len);
        int ftruncate(FD fd, off_t length);

        FD eventfd2(unsigned int initval, int flags);
        FD epoll_create1(int flags);

        FD socket(int domain, int type, int protocol);
        int connect(FD sockfd, const Buffer& buffer);
        int bind(FD sockfd, const Buffer& name);
        int shutdown(FD sockfd, int how);
        ErrnoOrBuffer getpeername(FD sockfd, u32 buffersize);
        ErrnoOrBuffer getsockname(FD sockfd, u32 buffersize);
        ErrnoOrBuffer getsockopt(FD sockfd, int level, int optname, const Buffer& buffer);
        int setsockopt(FD sockfd, int level, int optname, const Buffer& buffer);

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress);
        ssize_t recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags);
        ssize_t send(FD sockfd, const Buffer& buffer, int flags);
        ssize_t sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags);

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
        File* tryGetFile(const Path& path);
        std::unique_ptr<File> tryTakeFile(const Path& path);
        Directory* ensurePathImpl(Span<const std::string> components);

        struct OpenNode {
            FD fd { -1 };
            std::string path;
            OpenFileDescription* openFiledescription;
        };

        void createStandardStreams();
        void findCurrentWorkDirectory();
        FD insertNode(std::unique_ptr<File> file);
        FD openNode(File* filePtr);
        FD allocateFd();
        FD insertNodeWithFd(std::unique_ptr<File> file, FD fd);

        OpenFileDescription* findOpenFileDescription(FD fd);

        Kernel& kernel_;
        std::unique_ptr<Directory> root_;
        TtyDevice* tty_ { nullptr };
        std::vector<std::unique_ptr<File>> orphanFiles_;
        std::vector<std::unique_ptr<Pipe>> pipes_;
        Directory* currentWorkDirectory_ { nullptr };
        std::deque<OpenFileDescription> openFileDescriptions_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif