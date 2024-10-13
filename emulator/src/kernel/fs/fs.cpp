#include "kernel/fs/fs.h"
#include "kernel/fs/epoll.h"
#include "kernel/fs/event.h"
#include "kernel/fs/regularfile.h"
#include "kernel/fs/hostfile.h"
#include "kernel/fs/shadowfile.h"
#include "kernel/fs/socket.h"
#include "kernel/fs/stream.h"
#include "kernel/kernel.h"
#include "verify.h"
#include <algorithm>
#include <fcntl.h>
#include <sys/poll.h>

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
        verify(stdinFd.fd == 0, "stdin must have fd 0");
        verify(stdoutFd.fd == 1, "stdout must have fd 1");
        verify(stderrFd.fd == 2, "stderr must have fd 2");
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
        FD fd = allocateFd();
        openFiles_.push_back(OpenNode{fd, nodePtr->path, nodePtr->object.get()});
        nodePtr->object->ref();
        return fd;
    }

    FS::FD FS::allocateFd() {
        // assign the file descriptor
        auto it = std::max_element(openFiles_.begin(), openFiles_.end(), [](const auto& a, const auto& b) {
            return a.fd.fd < b.fd.fd;
        });
        int fd = (it == openFiles_.end()) ? 0 : it->fd.fd+10;

        return FD{fd};
    }

    FS::FD FS::insertNodeWithFd(Node node, FD fd) {
        OpenNode* nodeWithExistingFd = findOpenNode(fd);
        verify(!nodeWithExistingFd, [&]() {
            fmt::print("cannot insert node with existing fd {}\n", fd.fd);
        });
        FD givenFd = insertNode(std::move(node));
        OpenNode* openNode = findOpenNode(givenFd);
        verify(!!openNode);
        openNode->fd = fd;
        return fd;
    }

    FS::FD FS::open(const std::string& path, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        if(path.empty()) return FD{-ENOENT};
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;

        verify(std::none_of(openFiles_.begin(), openFiles_.end(), [&](const OpenNode& openNode) {
            return openNode.path == path;
        }), "FS: opening same file twice is not supported");

        for(auto& node : files_) {
            if(node.path != path) continue;
            FD fd = allocateFd();
            node.object->ref();
            openFiles_.push_back(OpenNode{fd, node.path, node.object.get()});
            return fd;
        }

        if(canUseHostFile) {
            // open the file
            auto hostBackedFile = HostFile::tryCreate(this, path);
            if(!hostBackedFile) {
                // TODO: return the actual value of errno
                return FS::FD{-ENOENT};
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

    FS::FD FS::dup(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return FD{-EBADF};
        FD newFd = allocateFd();
        openNode->object->ref();
        openFiles_.push_back(OpenNode{newFd, openNode->path, openNode->object});
        return newFd;
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
        if(!openNode->object->isFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        verify(!!file, "unexpected nullptr");
        return file->read(count);
    }

    ErrnoOrBuffer FS::pread(FD fd, size_t count, off_t offset) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer{-EBADF};
        if(!openNode->object->isFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        verify(!!file, "unexpected nullptr");
        return file->pread(count, offset);
    }

    ssize_t FS::write(FD fd, const u8* buf, size_t count) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isFile()) return -EBADF;
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        verify(!!file, "unexpected nullptr");
        return file->write(buf, count);
    }

    ssize_t FS::pwrite(FD fd, const u8* buf, size_t count, off_t offset) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isFile()) return -EBADF;
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        verify(!!file, "unexpected nullptr");
        return file->pwrite(buf, count, offset);
    }

    ssize_t FS::writev(FD fd, const std::vector<Buffer>& buffers) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isWritable()) return -EBADF;
        ssize_t nbytes = 0;
        for(const Buffer& buf : buffers) {
            ssize_t ret = openNode->object->write(buf.data(), buf.size());
            if(ret < 0) return ret;
            nbytes += ret;
        }
        return nbytes;
    }

    ErrnoOrBuffer FS::stat(const std::string& path) {
        for(auto& node : files_) {
            if(node.path != path) continue;
            if(node.object->isFile()) {
                RegularFile* file = static_cast<RegularFile*>(node.object.get());
                return file->stat();
            }
            verify(false, "implement stat for non files in FS");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        return kernel_.host().stat(path);
    }

    ErrnoOrBuffer FS::fstat(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        if(!openNode->object->isFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        return file->stat();
    }

    ErrnoOrBuffer FS::statx(const std::string& path, int flags, unsigned int mask) {
        for(auto& node : files_) {
            if(node.path != path) continue;
            if(node.object->isFile()) {
                verify(false, "implement statx for files in FS");
                // RegularFile* file = static_cast<RegularFile*>(node.object.get());
                // return file->statx(flags, mask);
            }
            verify(false, "implement statx for non files in FS");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        return kernel_.host().statx(Host::cwdfd(), path, flags, mask);
    }

    ErrnoOrBuffer FS::fstatat64(FD dirfd, const std::string& path, int flags) {
        if(flags == AT_EMPTY_PATH) {
            return fstat(dirfd);
        }
        verify(dirfd.fd == Host::cwdfd().fd, "dirfd is not cwd");
        for(auto& node : files_) {
            if(node.path != path) continue;
            if(node.object->isFile()) {
                verify(false, "implement fstatat for files in FS");
                // RegularFile* file = static_cast<RegularFile*>(node.object.get());
                // return file->fstatat(flags, mask);
            }
            verify(false, "implement fstatat for non files in FS");
            return ErrnoOrBuffer(-ENOTSUP);
        }
        return kernel_.host().fstatat64(Host::cwdfd(), path, flags);
    }

    off_t FS::lseek(FD fd, off_t offset, int whence) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isFile()) return -EBADF;
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        return file->lseek(offset, whence);
    }

    int FS::close(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        OpenNode node = *openNode;
        node.object->unref();
        node.object->close();
        openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& openNode) {
            return openNode.fd == fd;
        }), openFiles_.end());
        if(node.object->refCount() == 0 && !node.object->keepAfterClose()) {
            files_.erase(std::remove_if(files_.begin(), files_.end(), [&](const auto& f) {
                return f.object.get() == node.object;
            }), files_.end());
        }
        return 0;
    }

    ErrnoOrBuffer FS::getdents64(FD fd, size_t count) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        if(!openNode->object->isFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        return file->getdents64(count);
    }

    int FS::fcntl(FD fd, int cmd, int arg) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return -EBADF;
        if(openNode->object->isFile()) {
            RegularFile* file = static_cast<RegularFile*>(openNode->object);
            return file->fcntl(cmd, arg);
        }
        if(openNode->object->isSocket()) {
            Socket* socket = static_cast<Socket*>(openNode->object);
            return socket->fcntl(cmd, arg);
        }
        return -EBADF;
    }

    ErrnoOrBuffer FS::ioctl(FD fd, unsigned long request, const Buffer& buffer) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        if(!openNode->object->isFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openNode->object);
        return file->ioctl(request, buffer);
    }

    FS::FD FS::eventfd2(unsigned int initval, int flags) {
        std::unique_ptr<Event> event = Event::tryCreate(this, initval, flags);
        return insertNode(Node {
            "",
            std::move(event),
        });
    }

    FS::FD FS::epoll_create1(int flags) {
        std::unique_ptr<Epoll> epoll = std::make_unique<Epoll>(this, flags);
        return insertNode(Node {
            "",
            std::move(epoll),
        });
    }

    FS::FD FS::socket(int domain, int type, int protocol) {
        auto socket = Socket::tryCreate(this, domain, type, protocol);
        // TODO: return the actual value of errno
        if(!socket) return FS::FD{-EINVAL};
        return insertNode(Node {
            "",
            std::move(socket)
        });
    }

    int FS::connect(FD sockfd, const Buffer& buffer) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->connect(buffer);
    }

    int FS::bind(FD sockfd, const Buffer& name) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->bind(name);
    }

    int FS::shutdown(FD sockfd, int how) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->shutdown(how);
    }

    ErrnoOrBuffer FS::getpeername(FD sockfd, u32 buffersize) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        if(!openNode->object->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->getpeername(buffersize);
    }

    ErrnoOrBuffer FS::getsockname(FD sockfd, u32 buffersize) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return ErrnoOrBuffer(-EBADF);
        if(!openNode->object->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->getsockname(buffersize);
    }

    ErrnoOr<BufferAndReturnValue<int>> FS::poll(const Buffer& buffer, u64 nfds, int timeout) {
        std::vector<pollfd> virtualPollFds;
        assert(buffer.size() % sizeof(pollfd) == 0);
        virtualPollFds.resize(buffer.size() / sizeof(pollfd));
        std::memcpy(virtualPollFds.data(), buffer.data(), buffer.size());

        // check that all fds are pollable and have a host-side fd
        for(pollfd vpfd : virtualPollFds) {
            OpenNode* openNode = findOpenNode(FD{vpfd.fd});
            verify(!!openNode, [&]() { fmt::print("cannot poll fd={}\n", vpfd.fd); });
            verify(openNode->object->isPollable(), [&]() { fmt::print("fd={} is not pollable\n", vpfd.fd); });
            auto hostFd = openNode->object->hostFileDescriptor();
            verify(hostFd.has_value(), [&]() { fmt::print("fd={} has no host-equivalent fd\n", vpfd.fd); });
        }

        // substitute the virtual fds with host fds
        std::vector<pollfd> hostPollFds;
        for(pollfd vpfd : virtualPollFds) {
            OpenNode* openNode = findOpenNode(FD{vpfd.fd});
            pollfd hostPollFd = vpfd;
            auto hostFd = openNode->object->hostFileDescriptor();
            hostPollFd.fd = *hostFd;
            hostPollFds.push_back(hostPollFd);
        }

        // call poll
        int ret = ::poll(hostPollFds.data(), nfds, timeout);
        if(ret < 0) return ErrnoOr<BufferAndReturnValue<int>>(-errno);

        // transfer the events to the virtual fds
        for(size_t i = 0; i < virtualPollFds.size(); ++i) {
            virtualPollFds[i].events = hostPollFds[i].events;
            virtualPollFds[i].revents = hostPollFds[i].revents;
        }
        BufferAndReturnValue<int> bufferAndRetVal {
            Buffer{std::move(virtualPollFds)},
            ret,
        };
        return ErrnoOr<BufferAndReturnValue<int>>(std::move(bufferAndRetVal));
    }

    ErrnoOr<std::pair<Buffer, Buffer>> FS::recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        if(!openNode->object->isSocket()) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->recvfrom(len, flags, requireSrcAddress);

    }
    ssize_t FS::recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->recvmsg(flags, msg_name, msg_iov, msg_control, msg_flags);
    }

    ssize_t FS::sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags) {
        OpenNode* openNode = findOpenNode(sockfd);
        if(!openNode) return -EBADF;
        if(!openNode->object->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openNode->object);
        return socket->sendmsg(flags, msg_name, msg_iov, msg_control, msg_flags);
    }

    std::string FS::filename(FD fd) {
        OpenNode* openNode = findOpenNode(fd);
        if(!openNode) return "";
        return openNode->path;
    }

}