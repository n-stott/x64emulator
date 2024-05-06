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
        FD stdinFd = insertNodeWithFd(Node {
            "__stdin",
            std::make_unique<Stream>(this, Stream::TYPE::IN),
        }, FD{0});
        FD stdoutFd = insertNodeWithFd(Node {
            "__stdout",
            std::make_unique<Stream>(this, Stream::TYPE::OUT),
        }, FD{1});
        FD stderrFd = insertNodeWithFd(Node {
            "__stderr",
            std::make_unique<Stream>(this, Stream::TYPE::ERR),
        }, FD{2});
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
        return allocateFd(nodePtr);
    }

    FS::FD FS::allocateFd(Node* node) {
        // assign the file descriptor
        auto it = std::max_element(openFiles_.begin(), openFiles_.end(), [](const auto& a, const auto& b) {
            return a.fd.fd < b.fd.fd;
        });
        int fd = (it == openFiles_.end()) ? 0 : it->fd.fd+10;

        openFiles_.push_back(OpenNode{fd, node});

        return FD{fd};
    }

    FS::FD FS::insertNodeWithFd(Node node, FD fd) {
        OpenNode* nodeWithExistingFd = findOpenNode(fd);
        x64::verify(!nodeWithExistingFd, [&]() {
            fmt::print("cannot insert node with existing fd {}\n", fd.fd);
        });
        FD givenFd = insertNode(std::move(node));
        OpenNode* openNode = findOpenNode(givenFd);
        x64::verify(!!openNode);
        openNode->fd = fd;
        return fd;
    }

    FS::FD FS::open(const std::string& path, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        x64::verify(!path.empty(), "FS::open: empty path");
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;

        x64::verify(std::none_of(openFiles_.begin(), openFiles_.end(), [&](const OpenNode& openNode) {
            return openNode.node->path == path;
        }), "FS: opening same file twice is not supported");

        for(auto& node : files_) {
            if(node.path != path) continue;
            return allocateFd(&node);
        }

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

    FS::OpenNode* FS::findOpenNode(FD fd) {
        for(OpenNode& node : openFiles_) {
            if(node.fd != fd) continue;
            return &node;
        }
        return nullptr;
    }

    ErrnoOrBuffer FS::read(FD fd, size_t count) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer{-EBADF};
        x64::verify(!!openNode->node, "unexpected nullptr");
        File* file = openNode->node->file.get();
        x64::verify(!!file, "unexpected nullptr");
        return file->read(count);
    }

    ErrnoOrBuffer FS::pread(FD fd, size_t count, size_t offset) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer{-EBADF};
        x64::verify(!!openNode->node, "unexpected nullptr");
        File* file = openNode->node->file.get();
        x64::verify(!!file, "unexpected nullptr");
        return file->pread(count, offset);
    }

    ssize_t FS::write(FD fd, const u8* buf, size_t count) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        x64::verify(!!openNode->node, "unexpected nullptr");
        File* file = openNode->node->file.get();
        x64::verify(!!file, "unexpected nullptr");
        return file->write(buf, count);
    }

    ssize_t FS::pwrite(FD fd, const u8* buf, size_t count, size_t offset) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        x64::verify(!!openNode->node, "unexpected nullptr");
        File* file = openNode->node->file.get();
        x64::verify(!!file, "unexpected nullptr");
        return file->pwrite(buf, count, offset);
    }

    ErrnoOrBuffer FS::stat(const std::string& path) {
        for(auto& node : files_) {
            if(node.path != path) continue;
            x64::verify(false, "implement stat in FS");
            return ErrnoOrBuffer(-ENOTSUP);
            // return node.file->stat();
        }
        return kernel_.host().stat(path);
    }

    ErrnoOrBuffer FS::fstat(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        return openNode->node->file->stat();
    }

    int FS::close(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        OpenNode file = *openNode;
        openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& openNode) {
            return openNode.fd == fd;
        }), openFiles_.end());
        file.node->file->close();
        if(!file.node->file->keepAfterClose()) {
            files_.erase(std::remove_if(files_.begin(), files_.end(), [&](const auto& f) {
                return &f == file.node;
            }), files_.end());
        }
        return 0;
    }

}