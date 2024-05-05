#include "fs/fs.h"
#include "fs/file.h"
#include "fs/hostfile.h"
#include "fs/shadowfile.h"
#include "fs/stream.h"
#include "interpreter/kernel.h"
#include "interpreter/verify.h"

namespace kernel {

    FS::FS(Kernel& kernel) : kernel_(kernel) {
        createStandardStreams();
    }

    void FS::createStandardStreams() {
        FD stdinFd = insertNode(Node {
            "__stdin",
            std::make_unique<Stream>(this, Stream::TYPE::IN),
        });
        FD stdoutFd = insertNode(Node {
            "__stdout",
            std::make_unique<Stream>(this, Stream::TYPE::OUT),
        });
        FD stderrFd = insertNode(Node {
            "__stderr",
            std::make_unique<Stream>(this, Stream::TYPE::ERR),
        });
        x64::verify(stdinFd.fd == 0, "stdin must have fd 0");
        x64::verify(stdoutFd.fd == 1, "stdout must have fd 1");
        x64::verify(stderrFd.fd == 2, "stderr must have fd 2");
    }

    FS::~FS() = default;

    FS::OpenFlags FS::fromFlags(int flags) {
        return OpenFlags {
            Host::Open::isReadable(flags),
            Host::Open::isWritable(flags),
            Host::Open::isAppending(flags),
            Host::Open::isTruncating(flags),
            Host::Open::isCreatable(flags),
            Host::Open::isCloseOnExec(flags),
        };
    }

    FS::Permissions FS::fromMode(unsigned int mode) {
        return Permissions {
            Host::Mode::isUserReadable(mode),
            Host::Mode::isUserWritable(mode),
            Host::Mode::isUserExecutable(mode),
        };
    }

    FS::FD FS::insertNode(Node node) {
        files_.push_back(std::move(node));
        Node* nodePtr = &files_.back();

        // assign the file descriptor
        auto it = std::max_element(openFiles_.begin(), openFiles_.end(), [](const auto& a, const auto& b) {
            return a.fd.fd < b.fd.fd;
        });
        int fd = (it == openFiles_.end()) ? 0 : it->fd.fd+1;

        openFiles_.push_back(OpenNode{fd, nodePtr});

        return FD{fd};
    }

    FS::FD FS::open(const std::string& path, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        x64::verify(!path.empty(), "FS::open: empty path");
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;

        if(canUseHostFile) {
            // open the file
            auto hostBackedFile = HostFile::tryCreate(this, path);
            if(!hostBackedFile) {
                // TODO: return the actual value of errno
                return FS::FD{-EINVAL};
            }
            
            // create and add the node to the filesystem
            return insertNode(Node {
                path,
                std::move(hostBackedFile),
            });
        } else {
            // open the file
            auto shadowFile = ShadowFile::tryCreate(this, path, flags.create);
            if(!shadowFile) {
                // TODO: return the actual value of errno
                return FS::FD{-EINVAL};
            }

            if(flags.truncate) shadowFile->truncate();
            if(flags.append) shadowFile->append();
            shadowFile->setWritable(flags.write);
            
            // create and add the node to the filesystem
            return insertNode(Node {
                path,
                std::move(shadowFile),
            });
        }
        return FD{-EINVAL};
    }

}