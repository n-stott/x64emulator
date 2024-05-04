#ifndef FS_H
#define FS_H

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace kernel {

    class File;
    class Host;

    class FS {
    public:
        explicit FS(Host* host);
        ~FS();

        struct FD {
            int fd;
        };

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
            bool useExecutable { false };
        };

        static OpenFlags fromFlags(int flags);
        static Permissions fromMode(unsigned int mode);

        FD open(FD dirfd, const std::string& path, OpenFlags flags, Permissions permissions);

    private:
        struct Node {
            std::string path;
            std::unique_ptr<File> file;
        };

        struct OpenNode {
            Node* node;
            OpenFlags flags;
            Permissions permissions;
        };

        Host* host_ { nullptr };
        std::deque<Node> existingFiles_;
        std::vector<OpenNode> openFiles_;
    };

}

#endif