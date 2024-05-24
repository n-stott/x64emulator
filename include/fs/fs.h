#ifndef FS_H
#define FS_H

#include "utils/buffer.h"
#include "utils/erroror.h"
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    class FsObject;
    class File;
    class Kernel;

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

        FD open(const std::string& path, OpenFlags flags, Permissions permissions);
        FD dup(FD fd);
        int close(FD fd);

        ErrnoOrBuffer read(FD fd, size_t count);
        ErrnoOrBuffer pread(FD fd, size_t count, off_t offset);

        ssize_t write(FD fd, const u8* buf, size_t count);
        ssize_t pwrite(FD fd, const u8* buf, size_t count, off_t offset);
        ssize_t writev(FD fd, const std::vector<Buffer>& buffers);

        ErrnoOrBuffer stat(const std::string& path);
        ErrnoOrBuffer fstat(FD fd);
        ErrnoOrBuffer statx(const std::string& path, int flags, unsigned int mask);
        ErrnoOrBuffer fstatat64(FD dirfd, const std::string& path, int flags);

        off_t lseek(FD fd, off_t offset, int whence);

        ErrnoOrBuffer getdents64(FD fd, size_t count);
        int fcntl(FD fd, int cmd, int arg);
        ErrnoOrBuffer ioctl(FD fd, unsigned long request, const Buffer& buffer);

        FD eventfd2(unsigned int initval, int flags);
        FD epoll_create1(int flags);

        FD socket(int domain, int type, int protocol);
        int connect(FD sockfd, const Buffer& buffer);
        int bind(FD sockfd, const Buffer& name);
        int shutdown(FD sockfd, int how);
        ErrnoOrBuffer getpeername(FD sockfd, u32 buffersize);
        ErrnoOrBuffer getsockname(FD sockfd, u32 buffersize);

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress);
        ssize_t recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags);
        ssize_t sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags);
        
        ErrnoOr<BufferAndReturnValue<int>> poll(const Buffer&, u64 nfds, int timeout);

        std::string filename(FD fd);

    private:
        struct Node {
            std::string path;
            std::unique_ptr<FsObject> object;
        };

        struct OpenNode {
            FD fd { -1 };
            std::string path;
            FsObject* object { nullptr };
        };

        void createStandardStreams();
        FD insertNode(Node node);
        FD allocateFd();
        FD insertNodeWithFd(Node node, FD fd);

        OpenNode* findOpenNode(FD fd);

        Kernel& kernel_;
        std::deque<Node> files_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif