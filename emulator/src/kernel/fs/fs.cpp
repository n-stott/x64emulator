#include "kernel/fs/fs.h"
#include "kernel/fs/openfiledescription.h"
#include "kernel/fs/epoll.h"
#include "kernel/fs/event.h"
#include "kernel/fs/regularfile.h"
#include "kernel/fs/hostdirectory.h"
#include "kernel/fs/hostfile.h"
#include "kernel/fs/path.h"
#include "kernel/fs/pipe.h"
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
        root_ = HostDirectory::tryCreateRoot(this);
        verify(!!root_, "Unable to create root directory");
        createStandardStreams();
        findCurrentWorkDirectory();
    }
    

    void FS::createStandardStreams() {
        FD stdinFd = insertNodeWithFd(std::make_unique<Stream>(this, Stream::TYPE::IN), FD{0});
        FD stdoutFd = insertNodeWithFd(std::make_unique<Stream>(this, Stream::TYPE::OUT), FD{1});
        FD stderrFd = insertNodeWithFd(std::make_unique<Stream>(this, Stream::TYPE::ERR), FD{2});
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
        currentWorkDirectory_ = ensureCompletePath(*cwdPath);
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

    FS::FD FS::insertNode(std::unique_ptr<File> file) {
        File* filePtr = file.get();
        orphanFiles_.push_back(std::move(file));
        FD fd = allocateFd();
        openFileDescriptions_.push_back(OpenFileDescription(filePtr, {}));
        openFiles_.push_back(OpenNode{fd, "", &openFileDescriptions_.back()});
        filePtr->ref();
        return fd;
    }

    FS::FD FS::openNode(File* filePtr) {
        FD fd = allocateFd();
        openFileDescriptions_.push_back(OpenFileDescription(filePtr, {}));
        openFiles_.push_back(OpenNode{fd, "", &openFileDescriptions_.back()});
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

    FS::FD FS::insertNodeWithFd(std::unique_ptr<File> file, FD fd) {
        OpenFileDescription* openFileDescriptionWithExistingFD = findOpenFileDescription(fd);
        verify(!openFileDescriptionWithExistingFD, [&]() {
            fmt::print("cannot insert node with existing fd {}\n", fd.fd);
        });
        FD givenFd = insertNode(std::move(file));
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

    Directory* FS::ensureCompletePath(const Path& path) {
        return ensurePathImpl(path.components());
    }

    Directory* FS::ensurePathExceptLast(const Path& path) {
        return ensurePathImpl(path.componentsExceptLast());
    }

    Directory* FS::ensurePathImpl(Span<const std::string> components) {
        Directory* dir = root();
        for(const std::string& component : components) {
            // First, look if it is already there
            Directory* subdir = dir->tryGetSubDirectory(component);
            if(!!subdir) {
                dir = subdir;
                continue;
            }
            // Then try to get it on the host
            Directory* hostSubdir = dir->tryAddHostDirectory(component);
            if(!!hostSubdir) {
                dir = hostSubdir;
                continue;
            }
            // Finally, resort to creating a shadow version
            Directory* shadowSubdir = dir->tryAddShadowDirectory(component);
            verify(!!shadowSubdir, "Unable to create shadow subdirectory");
            dir = shadowSubdir;
        }
        return dir;
    }

    File* FS::tryGetFile(const Path& path) {
        Directory* dir = root();
        for(const std::string& component : path.componentsExceptLast()) {
            dir = dir->tryGetSubDirectory(component);
            if(!dir) return nullptr;
        }
        File* file = dir->tryGetEntry(path.last());
        return file;
    }

    std::unique_ptr<File> FS::tryTakeFile(const Path& path) {
        Directory* dir = root();
        for(const std::string& component : path.componentsExceptLast()) {
            dir = dir->tryGetSubDirectory(component);
            if(!dir) return nullptr;
        }
        std::unique_ptr<File> file = dir->tryTakeEntry(path.last());
        return file;
    }

    FS::FD FS::open(const std::string& pathname, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        if(pathname.empty()) return FD{-ENOENT};
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;

        // Look if the file is already present in FS, open or closed.
        auto absolutePathname = toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        
        File* file = tryGetFile(*path);
        if(!!file) {
            FD fd = allocateFd();
            file->ref();
            openFileDescriptions_.push_back(OpenFileDescription { file, {} });
            openFiles_.push_back(OpenNode{fd, "", &openFileDescriptions_.back()});
            file->open();
            return fd;
        }

        if(canUseHostFile) {
            // open the file
            auto* hostBackedFile = HostFile::tryCreateAndAdd(this, root_.get(), absolutePathname);
            if(!hostBackedFile) {
                // TODO: return the actual value of errno
                return FS::FD{-ENOENT};
            }
            
            // create and add the node to the filesystem
            hostBackedFile->open();
            return openNode(hostBackedFile);
        } else {
            // open the file
            auto* shadowFile = ShadowFile::tryCreateAndAdd(this, root_.get(), absolutePathname, flags.create);
            if(!shadowFile) {
                // TODO: return the actual value of errno
                return FS::FD{-EINVAL};
            }

            if(flags.truncate) shadowFile->truncate();
            if(flags.append) shadowFile->append();
            shadowFile->setWritable(flags.write);
            
            // create and add the node to the filesystem
            shadowFile->open();
            return openNode(shadowFile);
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

    int FS::mkdir(const std::string& pathname) {
        auto path = Path::tryCreate(pathname);
        if(!path) return -ENOENT;
        ensureCompletePath(*path);
        return 0;
    }

    int FS::rename(const std::string& oldname, const std::string& newname) {
        auto oldpath = Path::tryCreate(oldname);
        auto newpath = Path::tryCreate(newname);
        if(!oldpath) return -ENOENT;
        if(!newpath) return -ENOENT;
        auto file = tryTakeFile(*oldpath);
        if(!file) return -ENOENT;
        auto* newdir = ensurePathExceptLast(*newpath);
        verify(!!newdir, "Unable to create new directory");
        newdir->addFile(std::move(file));
        return 0;
    }
    
    int FS::unlink(const std::string& pathname) {
        auto path = Path::tryCreate(toAbsolutePathname(pathname));
        if(!path) return -ENOENT;
        File* filePtr = tryGetFile(*path);
        if(!filePtr) return -ENOENT;
        if(filePtr->refCount() > 0) {
            filePtr->setDeleteAfterClose();
        } else {
            auto file = tryTakeFile(*path);
            // let the file die  here
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
        auto path = Path::tryCreate(pathname);
        if(!!path) {
            File* file = tryGetFile(*path);
            if(!!file) return file->stat();
        }
        return kernel_.host().stat(pathname);
    }

    ErrnoOrBuffer FS::fstat(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->stat();
    }

    ErrnoOrBuffer FS::statx(const std::string& pathname, int flags, unsigned int mask) {
        auto path = Path::tryCreate(pathname);
        if(!!path) {
            verify(false, "implement statx in FS");
            return ErrnoOrBuffer(-ENOTSUP);
        } else {
            return kernel_.host().statx(Host::cwdfd(), pathname, flags, mask);
        }
        
    }

    ErrnoOrBuffer FS::fstatat64(FD dirfd, const std::string& pathname, int flags) {
        if(flags == AT_EMPTY_PATH) {
            return fstat(dirfd);
        }
        verify(dirfd.fd == Host::cwdfd().fd, "dirfd is not cwd");
        auto path = Path::tryCreate(pathname);
        if(!!path) {
            verify(false, "implement fstatat in FS");
            return ErrnoOrBuffer(-ENOTSUP);
        } else {
            return kernel_.host().fstatat64(Host::cwdfd(), pathname, flags);
        }
    }

    off_t FS::lseek(FD fd, off_t offset, int whence) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        return openFileDescription->lseek(offset, whence);
    }

    int FS::close(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        file->unref();
        file->close();
        openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& openNode) {
            return openNode.fd == fd;
        }), openFiles_.end());
        if(file->refCount() == 0 && !file->keepAfterClose()) {
            unlink(file->path());
        }
        return 0;
    }

    ErrnoOrBuffer FS::getdents64(FD fd, size_t count) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        File* file = openFileDescription->file();
        return file->getdents64(count);
    }

    int FS::fcntl(FD fd, int cmd, int arg) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        return file->fcntl(cmd, arg);
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
        return insertNode(std::move(event));
    }

    FS::FD FS::epoll_create1(int flags) {
        std::unique_ptr<Epoll> epoll = std::make_unique<Epoll>(this, flags);
        return insertNode(std::move(epoll));
    }

    FS::FD FS::socket(int domain, int type, int protocol) {
        auto socket = Socket::tryCreate(this, domain, type, protocol);
        // TODO: return the actual value of errno
        if(!socket) return FS::FD{-EINVAL};
        return insertNode(std::move(socket));
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
            if(!openFileDescription) {
                rfd.revents = rfd.revents | PollEvent::INVALID_REQUEST;
                continue;
            }
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

    ErrnoOr<std::pair<FS::FD, FS::FD>> FS::pipe2(int flags) {
        using ReturnType = ErrnoOr<std::pair<FS::FD, FS::FD>>;
        auto pipe = Pipe::tryCreate(this, flags);
        if(!pipe) return ReturnType(-EMFILE);

        auto readingSide = pipe->tryCreateReader();
        if(!readingSide) return ReturnType(-EMFILE);

        auto writingSide = pipe->tryCreateWriter();
        if(!writingSide) return ReturnType(-EMFILE);

        pipes_.push_back(std::move(pipe));
        FD reader = insertNode(std::move(readingSide));
        FD writer = insertNode(std::move(writingSide));

        return ReturnType(std::make_pair(reader, writer));
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
        if(!openFileDescription->file()->isSocket()) return -ENOTSOCK;
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
        fmt::print("Open files:\n");
        for(const auto& openFile : openFiles_) {
            fmt::print("  fd={} : type={:20} path={}\n", openFile.fd.fd, openFile.openFiledescription->toString(), openFile.path);
        }
        root_->printSubtree();
    }

}