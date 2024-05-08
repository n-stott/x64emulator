#ifndef FS_H
#define FS_H

#include "utils/buffer.h"
#include "utils/erroror.h"
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace kernel {

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
        int close(FD fd);

        ErrnoOrBuffer read(FD fd, size_t count);
        ErrnoOrBuffer pread(FD fd, size_t count, off_t offset);

        ssize_t write(FD fd, const u8* buf, size_t count);
        ssize_t pwrite(FD fd, const u8* buf, size_t count, off_t offset);
        ssize_t writev(FD fd, const std::vector<Buffer>& buffers);

        ErrnoOrBuffer stat(const std::string& path);
        ErrnoOrBuffer fstat(FD fd);

        off_t lseek(FD fd, off_t offset, int whence);

        ErrnoOrBuffer getdents64(FD fd, size_t count);
        int fcntl(FD fd, int cmd, int arg);

        std::string filename(FD fd);

    private:
        struct Node {
            std::string path;
            std::unique_ptr<File> file;
        };

        struct OpenNode {
            FD fd { -1 };
            std::string path;
            File* file { nullptr };
        };

        void createStandardStreams();
        FD insertNode(Node node);
        FD allocateFd(Node* node);
        FD insertNodeWithFd(Node node, FD fd);

        OpenNode* findOpenNode(FD fd);

        Kernel& kernel_;
        std::deque<Node> files_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif