#include "kernel/fs/fs.h"
#include "kernel/fs/openfiledescription.h"
#include "kernel/fs/epoll.h"
#include "kernel/fs/event.h"
#include "kernel/fs/regularfile.h"
#include "kernel/fs/hostfile.h"
#include "kernel/fs/path.h"
#include "kernel/fs/shadowfile.h"
#include "kernel/fs/socket.h"
#include "kernel/fs/stream.h"
#include "kernel/kernel.h"
#include "verify.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/file.h>

namespace kernel {

    FS::FS(Kernel& kernel) : kernel_(kernel) {
        root_ = std::make_unique<Directory>(this, nullptr, "");
        createStandardStreams();
        findCurrentWorkDirectory();
    }
    

    void FS::createStandardStreams() {
        FD stdinFd = insertNodeWithFd(FsNode {
            "__stdin",
            std::make_unique<Stream>(this, Stream::TYPE::IN),
        }, FD{0});
        FD stdoutFd = insertNodeWithFd(FsNode {
            "__stdout",
            std::make_unique<Stream>(this, Stream::TYPE::OUT),
        }, FD{1});
        FD stderrFd = insertNodeWithFd(FsNode {
            "__stderr",
            std::make_unique<Stream>(this, Stream::TYPE::ERR),
        }, FD{2});
        verify(stdinFd.fd == 0, "stdin must have fd 0");
        verify(stdoutFd.fd == 1, "stdout must have fd 1");
        verify(stderrFd.fd == 2, "stderr must have fd 2");
    }

    void FS::findCurrentWorkDirectory() {
        auto bufferOrError = kernel_.host().getcwd(1024);
        verify(!bufferOrError.isError());
        std::string cwdpathname;
        bufferOrError.errorOrWith<int>([&](const Buffer& buf) {
            cwdpathname = (char*)buf.data();
            return 0;
        });
        auto cwdPath = Path::tryCreate(cwdpathname);
        verify(!!cwdPath);
        currentWorkDirectory_ = ensurePath(*cwdPath);
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

    FS::FD FS::insertNode(FsNode node) {
        std::string path = node.path;
        files_.push_back(std::move(node));
        File* filePtr = files_.back().file.get();
        FD fd = allocateFd();
        openFileDescriptions_.push_back(OpenFileDescription(filePtr, {}));
        openFiles_.push_back(OpenNode{fd, std::move(path), &openFileDescriptions_.back()});
        filePtr->ref();
        return fd;
    }

    FS::FD FS::allocateFd() {
        // assign the file descriptor
        auto it = std::max_element(openFiles_.begin(), openFiles_.end(), [](const auto& a, const auto& b) {
            return a.fd.fd < b.fd.fd;
        });
        int fd = (it == openFiles_.end()) ? 0 : it->fd.fd+1;
        return FD{fd};
    }

    FS::FD FS::insertNodeWithFd(FsNode node, FD fd) {
        OpenFileDescription* openFileDescriptionWithExistingFD = findOpenFileDescription(fd);
        verify(!openFileDescriptionWithExistingFD, [&]() {
            fmt::print("cannot insert node with existing fd {}\n", fd.fd);
        });
        FD givenFd = insertNode(std::move(node));
        verify(givenFd == fd);
        return fd;
    }

    std::string FS::toAbsolutePathname(const std::string& pathname) const {
        verify(!pathname.empty(), "Empty pathname");
        if(pathname[0] == '/') {
            return pathname;
        } else {
            verify(!!currentWorkDirectory_, "No current work directory");
            return currentWorkDirectory_->path() + "/" + pathname;
        }
    }

    Directory* FS::ensurePath(const Path& path) {
        Directory* dir = root();
        for(const std::string& component : path.componentsExceptLast()) {
            dir = dir->tryGetOrAddSubDirectory(component);
        }
        return dir;
    }

    FS::FD FS::open(const std::string& pathname, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        if(pathname.empty()) return FD{-ENOENT};
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;

        // Look if the file is already present in FS, open or closed.
        for(auto& node : files_) {
            if(node.path != pathname) continue;
            FD fd = allocateFd();
            node.file->ref();
            openFileDescriptions_.push_back(OpenFileDescription { node.file.get(), {} });
            openFiles_.push_back(OpenNode{fd, node.path, &openFileDescriptions_.back()});
            return fd;
        }

        if(canUseHostFile) {
            // open the file
            auto hostBackedFile = HostFile::tryCreate(this, root_.get(), pathname);
            if(!hostBackedFile) {
                // TODO: return the actual value of errno
                return FS::FD{-ENOENT};
            }
            
            // create and add the node to the filesystem
            return insertNode(FsNode {
                pathname,
                std::move(hostBackedFile),
            });
        } else {
            // open the file
            auto shadowFile = ShadowFile::tryCreate(this, root_.get(), pathname, flags.create);
            if(!shadowFile) {
                // TODO: return the actual value of errno
                return FS::FD{-EINVAL};
            }

            if(flags.truncate) shadowFile->truncate();
            if(flags.append) shadowFile->append();
            shadowFile->setWritable(flags.write);
            
            // create and add the node to the filesystem
            return insertNode(FsNode {
                pathname,
                std::move(shadowFile),
            });
        }
        return FD{-EINVAL};
    }

    FS::FD FS::dup(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return FD{-EBADF};
        FD newFd = allocateFd();
        openFileDescription->file()->ref();
        std::string path = filename(fd);
        openFiles_.push_back(OpenNode{newFd, std::move(path), openFileDescription});
        return newFd;
    }

    FS::FD FS::dup2(FD oldfd, FD newfd) {
        OpenFileDescription* oldOfd = findOpenFileDescription(oldfd);
        if(!oldOfd) return FD{-EBADF};
        if(oldfd == newfd) return newfd;
        OpenFileDescription* newOfd = findOpenFileDescription(newfd);
        if(!!newOfd) {
            int ret = close(newfd);
            verify(ret == 0, "close in dup2 failed");
        }
        oldOfd->file()->ref();
        std::string path = filename(oldfd);
        openFiles_.push_back(OpenNode{newfd, std::move(path), oldOfd});
        return newfd;
    }
    
    int FS::unlink(const std::string& pathname) {
        auto it = std::find_if(files_.begin(), files_.end(), [&](const FsNode& node) {
            return node.path == pathname;
        });
        if(it == files_.end()) {
            return -ENOENT;
        }
        if(it->file->refCount() > 0) {
            it->file->setDeleteAfterClose();
        } else {
            files_.erase(it);
        }
        return 0;
    }

    OpenFileDescription* FS::findOpenFileDescription(FD fd) {
        for(OpenNode& node : openFiles_) {
            if(node.fd != fd) continue;
            return node.openFiledescription;
        }
        return nullptr;
    }

    ErrnoOrBuffer FS::read(FD fd, size_t count) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer{-EBADF};
        return openFileDescription->read(count);
    }

    ErrnoOrBuffer FS::pread(FD fd, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer{-EBADF};
        return openFileDescription->pread(count, offset);
    }

    ssize_t FS::write(FD fd, const u8* buf, size_t count) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        return openFileDescription->write(buf, count);
    }

    ssize_t FS::pwrite(FD fd, const u8* buf, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        return openFileDescription->pwrite(buf, count, offset);
    }

    ssize_t FS::writev(FD fd, const std::vector<Buffer>& buffers) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isWritable()) return -EBADF;
        ssize_t nbytes = 0;
        for(const Buffer& buf : buffers) {
            ssize_t ret = openFileDescription->write(buf.data(), buf.size());
            if(ret < 0) return ret;
            nbytes += ret;
        }
        return nbytes;
    }

    ErrnoOrBuffer FS::stat(const std::string& pathname) {
        for(auto& node : files_) {
            if(node.path != pathname) continue;
            return node.file->stat();
        }
        return kernel_.host().stat(pathname);
    }

    ErrnoOrBuffer FS::fstat(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->stat();
    }

    ErrnoOrBuffer FS::statx(const std::string& path, int flags, unsigned int mask) {
        for(auto& node : files_) {
            if(node.path != path) continue;
            if(node.file->isRegularFile()) {
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
            if(node.file->isRegularFile()) {
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
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        return openFileDescription->lseek(offset, whence);
    }

    int FS::close(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        openFileDescription->file()->unref();
        openFileDescription->file()->close();
        openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& openNode) {
            return openNode.fd == fd;
        }), openFiles_.end());
        if(openFileDescription->file()->refCount() == 0 && !openFileDescription->file()->keepAfterClose()) {
            files_.erase(std::remove_if(files_.begin(), files_.end(), [&](const auto& f) {
                return f.file.get() == openFileDescription->file();
            }), files_.end());
        }
        return 0;
    }

    ErrnoOrBuffer FS::getdents64(FD fd, size_t count) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isRegularFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openFileDescription->file());
        return file->getdents64(count);
    }

    int FS::fcntl(FD fd, int cmd, int arg) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        if(openFileDescription->file()->isRegularFile()) {
            RegularFile* file = static_cast<RegularFile*>(openFileDescription->file());
            return file->fcntl(cmd, arg);
        }
        if(openFileDescription->file()->isSocket()) {
            Socket* socket = static_cast<Socket*>(openFileDescription->file());
            return socket->fcntl(cmd, arg);
        }
        return -EBADF;
    }

    ErrnoOrBuffer FS::ioctl(FD fd, unsigned long request, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isRegularFile()) return ErrnoOrBuffer{-EBADF};
        RegularFile* file = static_cast<RegularFile*>(openFileDescription->file());
        return file->ioctl(request, buffer);
    }

    int FS::flock(FD fd, int operation) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        bool lockShared = (operation & LOCK_SH);
        bool lockExclusively = (operation & LOCK_EX);
        bool unlock = (operation & LOCK_UN);
        bool nonBlocking = (operation & LOCK_NB);
        OpenFileDescription::Blocking blocking
                = nonBlocking ? OpenFileDescription::Blocking::NO
                              : OpenFileDescription::Blocking::YES;
        if(lockExclusively) {
            if(lockShared) return -EINVAL;
            if(unlock) return -EINVAL;
            return openFileDescription->tryLock(OpenFileDescription::Lock::EXCLUSIVE, blocking);
        } else if(lockShared) {
            if(lockExclusively) return -EINVAL;
            if(unlock) return -EINVAL;
            return openFileDescription->tryLock(OpenFileDescription::Lock::EXCLUSIVE, blocking);
        } else if(unlock) {
            if(lockExclusively) return -EINVAL;
            if(lockShared) return -EINVAL;
            openFileDescription->unlock();
            return 0;
        }
        return -EINVAL;
    }

    FS::FD FS::eventfd2(unsigned int initval, int flags) {
        std::unique_ptr<Event> event = Event::tryCreate(this, initval, flags);
        return insertNode(FsNode {
            "",
            std::move(event),
        });
    }

    FS::FD FS::epoll_create1(int flags) {
        std::unique_ptr<Epoll> epoll = std::make_unique<Epoll>(this, flags);
        return insertNode(FsNode {
            "",
            std::move(epoll),
        });
    }

    FS::FD FS::socket(int domain, int type, int protocol) {
        auto socket = Socket::tryCreate(this, domain, type, protocol);
        // TODO: return the actual value of errno
        if(!socket) return FS::FD{-EINVAL};
        return insertNode(FsNode {
            "",
            std::move(socket)
        });
    }

    int FS::connect(FD sockfd, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->connect(buffer);
    }

    int FS::bind(FD sockfd, const Buffer& name) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->bind(name);
    }

    int FS::shutdown(FD sockfd, int how) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->shutdown(how);
    }

    ErrnoOrBuffer FS::getpeername(FD sockfd, u32 buffersize) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getpeername(buffersize);
    }

    ErrnoOrBuffer FS::getsockname(FD sockfd, u32 buffersize) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getsockname(buffersize);
    }

    ErrnoOr<BufferAndReturnValue<int>> FS::pollImmediate(const std::vector<PollData>& pfds) {
        std::vector<PollData> rfds = pfds;
        for(auto& rfd : rfds) {
            // check that all fds are pollable and have a host-side fd
            OpenFileDescription* openFileDescription = findOpenFileDescription(FD{rfd.fd});
            verify(!!openFileDescription, [&]() { fmt::print("cannot poll fd={}\n", rfd.fd); });
            File* file = openFileDescription->file();
            verify(file->isPollable(), [&]() { fmt::print("fd={} is not pollable\n", rfd.fd); });
            auto hostFd = file->hostFileDescriptor();
            verify(hostFd.has_value(), [&]() { fmt::print("fd={} has no host-equivalent fd\n", rfd.fd); });

            bool testRead = ((rfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ);
            bool testWrite = ((rfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE);
            if(testRead && file->canRead())   rfd.revents = rfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) rfd.revents = rfd.revents | PollEvent::CAN_WRITE;
        }
        warn("pollImmediate, errors not checked");
        int ret = 0; // TODO check errors
        BufferAndReturnValue<int> bufferAndRetVal {
            Buffer{std::move(rfds)},
            ret,
        };
        return ErrnoOr<BufferAndReturnValue<int>>(std::move(bufferAndRetVal));
    }

    void FS::doPoll(std::vector<PollData>* data) {
        if(!data) return;
        for(PollData& pollfd : *data) {
            FD fd { pollfd.fd };
            OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
            verify(!!openFileDescription, [&]() {
                fmt::print("Polling on non-existing open file description for fd={}\n", pollfd.fd);
            });
            File* file = openFileDescription->file();
            bool testRead = (pollfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ;
            bool testWrite = (pollfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE;
            if(testRead && file->canRead())   pollfd.revents = pollfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) pollfd.revents = pollfd.revents | PollEvent::CAN_WRITE;
        }
    }

    ErrnoOr<std::pair<Buffer, Buffer>> FS::recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->recvfrom(len, flags, requireSrcAddress);

    }
    ssize_t FS::recvmsg(FD sockfd, int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->recvmsg(flags, msg_name, msg_iov, msg_control, msg_flags);
    }

    ssize_t FS::send(FD sockfd, const Buffer& buffer, int flags) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->send(buffer, flags);
    }

    ssize_t FS::sendmsg(FD sockfd, int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->sendmsg(flags, msg_name, msg_iov, msg_control, msg_flags);
    }

    std::string FS::filename(FD fd) {
        for(const OpenNode& node : openFiles_) {
            if(node.fd != fd) continue;
            return node.path;
        }
        return "";
    }

    void FS::dumpSummary() const {
        fmt::print("Known files:\n");
        for(const auto& node : files_) {
            fmt::print("  type={:20} path={}\n", node.file->className(), node.path);
        }
        fmt::print("Open files:\n");
        for(const auto& openFile : openFiles_) {
            fmt::print("  fd={} : type={:20} path={}\n", openFile.fd.fd, openFile.openFiledescription->toString(), openFile.path);
        }
    }

}