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
        };

        FD open(const std::string& path, OpenFlags flags, Permissions permissions);

        ErrnoOrBuffer read(FD fd, size_t count);
        ErrnoOrBuffer pread(FD fd, size_t count, size_t offset);

        ssize_t write(FD fd, const u8* buf, size_t count);
        ssize_t pwrite(FD fd, const u8* buf, size_t count, size_t offset);

    private:
        struct Node {
            std::string path;
            std::unique_ptr<File> file;
        };

        struct OpenNode {
            FD fd { -1 };
            Node* node { nullptr };
        };

        void createStandardStreams();
        FD insertNode(Node node);

        OpenNode* findOpenNode(FD fd);

        Kernel& kernel_;
        std::deque<Node> files_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif