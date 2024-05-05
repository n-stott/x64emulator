#ifndef FS_H
#define FS_H

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

        Kernel& kernel_;
        std::deque<Node> files_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif