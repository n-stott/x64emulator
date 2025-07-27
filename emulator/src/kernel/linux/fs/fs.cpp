#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/procfs.h"
#include "kernel/linux/dev/hostdevice.h"
#include "kernel/linux/dev/shadowdevice.h"
#include "kernel/linux/dev/tty.h"
#include "kernel/linux/fs/openfiledescription.h"
#include "kernel/linux/fs/epoll.h"
#include "kernel/linux/fs/event.h"
#include "kernel/linux/fs/regularfile.h"
#include "kernel/linux/fs/hostdirectory.h"
#include "kernel/linux/fs/hostfile.h"
#include "kernel/linux/fs/hostsymlink.h"
#include "kernel/linux/fs/path.h"
#include "kernel/linux/fs/pipe.h"
#include "kernel/linux/fs/shadowfile.h"
#include "kernel/linux/fs/shadowsymlink.h"
#include "kernel/linux/fs/socket.h"
#include "kernel/linux/fs/symlink.h"
#include "host/host.h"
#include "verify.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <algorithm>

namespace kernel::gnulinux {

    FS::FS() {
        root_ = HostDirectory::tryCreateRoot(this);
        verify(!!root_, "Unable to create root directory");
        findCurrentWorkDirectory();
        tty_ = Tty::tryCreateAndAdd(this, root_.get(), "/dev/tty");
        createStandardStreams();
    }

    void FS::createStandardStreams() {
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<CreationFlags> creationFlags {};
        BitFlags<StatusFlags> statusFlags {};
        Permissions permissions;
        permissions.userReadable = true;
        permissions.userWriteable = true;
        FD stdinFd = open(FD{-100}, "/dev/tty", accessMode, creationFlags, statusFlags, permissions);
        FD stdoutFd = open(FD{-100}, "/dev/tty", accessMode, creationFlags, statusFlags, permissions);
        FD stderrFd = open(FD{-100}, "/dev/tty", accessMode, creationFlags, statusFlags, permissions);
        verify(stdinFd.fd == 0, "stdin must have fd 0");
        verify(stdoutFd.fd == 1, "stdout must have fd 1");
        verify(stderrFd.fd == 2, "stderr must have fd 2");
    }

    void FS::findCurrentWorkDirectory() {
        auto bufferOrError = Host::getcwd(1024);
        verify(!bufferOrError.isError());
        std::string cwdpathname;
        bufferOrError.errorOrWith<int>([&](const Buffer& buf) {
            cwdpathname = (char*)buf.data();
            return 0;
        });
        auto cwdPath = Path::tryCreate(cwdpathname);
        verify(!!cwdPath);
        currentWorkDirectory_ = ensureCompletePath(*cwdPath); // NOLINT(clang-analyzer-core.CallAndMessage)
    }

    void FS::resetProcFS(int pid, const std::string& programFilePath) {
        // only open files are stdin, stdout and stderr
        verify(openFiles_.size() == 3);

        // create a proc fs
        procfs_ = ProcFS::tryCreateAndAdd(this, root_.get(), "/proc");
        verify(!!procfs_, "Unable to create procFS");

        // create directory for that pid
        Directory* piddir = procfs_->tryAddShadowDirectory(fmt::format("{}", pid));
        verify(!!piddir, fmt::format("Unable to create /proc/{}", pid));

        // create symlink to that directory
        auto* selfsymlink = ShadowSymlink::tryCreateAndAdd(this, procfs_, "self", fmt::format("/proc/{}", pid));
        verify(!!selfsymlink, "Unable to create /proc/self");

        std::string absoluteProgramPath = toAbsolutePathname(programFilePath);

        // add symlink /proc/{pid}/exe to the executable path
        auto* exesymlink = ShadowSymlink::tryCreateAndAdd(this, piddir, "exe", absoluteProgramPath);
        verify(!!exesymlink, "Unable to create /proc/{pid}/exe");
    }

    FS::~FS() = default;

    BitFlags<FS::AccessMode> FS::toAccessMode(int flags) {
        BitFlags<AccessMode> am;
        if(Host::Open::isReadable(flags)) am.add(AccessMode::READ);
        if(Host::Open::isWritable(flags)) am.add(AccessMode::WRITE);
        return am;
    }

    BitFlags<FS::CreationFlags> FS::toCreationFlags(int flags) {
        BitFlags<CreationFlags> cf;
        if(Host::Open::isCloseOnExec(flags)) cf.add(CreationFlags::CLOEXEC);
        if(Host::Open::isCreatable(flags))   cf.add(CreationFlags::CREAT);
        if(Host::Open::isDirectory(flags))   cf.add(CreationFlags::DIRECTORY);
        // EXCL      // not supported
        // NOCTTY    // not supported
        // NOFOLLOW  // not supported
        // TMPFILE   // not supported
        if(Host::Open::isTruncating(flags))  cf.add(CreationFlags::TRUNC);
        return cf;
    }

    BitFlags<FS::StatusFlags> FS::toStatusFlags(int flags) {
        BitFlags<StatusFlags> sf;
        // Also update assembleAccessModeAndFileStatusFlags if more flags are supported
        if(Host::Open::isAppending(flags)) sf.add(StatusFlags::APPEND);
        // ASYNC     // not supported
        // DIRECT    // not supported
        // DSYNC     // not supported
        if(Host::Open::isLargeFile(flags)) sf.add(StatusFlags::LARGEFILE);
        // NOATIME   // not supported
        if(Host::Open::isNonBlock(flags))  sf.add(StatusFlags::NONBLOCK);
        // PATH      // not supported
        // SYNC      // not supported
        return sf;
    }

    FS::Permissions FS::fromMode(unsigned int mode) {
        return Permissions {
            Host::Mode::isUserReadable(mode),
            Host::Mode::isUserWritable(mode),
            Host::Mode::isUserExecutable(mode),
        };
    }

    FS::FD FS::insertNode(std::unique_ptr<File> file, BitFlags<AccessMode> accessMode, BitFlags<StatusFlags> statusFlags, bool closeOnExec) {
        File* filePtr = file.get();
        orphanFiles_.push_back(std::move(file));
        FD fd = allocateFd();
        openFileDescriptions_.push_back(OpenFileDescription(filePtr, accessMode, statusFlags));
        openFiles_.push_back(OpenNode{fd, &openFileDescriptions_.back(), closeOnExec});
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

    std::string FS::toAbsolutePathname(const std::string& pathname) const {
        verify(!pathname.empty(), "Empty pathname");
        if(pathname[0] == '/') {
            return pathname;
        } else {
            verify(!!currentWorkDirectory_, "No current work directory");
            return currentWorkDirectory_->path() + "/" + pathname;
        }
    }

    std::string FS::toAbsolutePathname(const std::string& pathname, FD dirfd) const {
        verify(!pathname.empty(), "Empty pathname");
        if(pathname[0] == '/') {
            return pathname;
        } else if(dirfd.fd == Host::cwdfd().fd) {
            return currentWorkDirectory_->path() + "/" + pathname;
        } else {
            OpenFileDescription* openFileDescription = const_cast<FS&>(*this).findOpenFileDescription(dirfd);
            verify(!!openFileDescription, "Trying to call toAbsolutePathname with unopened directory");
            verify(openFileDescription->file()->isDirectory(), "Trying to call toAbsolutePathname with non-directory");
            return openFileDescription->file()->path() + "/" + pathname;
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
            // If it's a symlink instead, resolve it
            File* file = dir->tryGetEntry(component);
            if(!!file && file->isSymlink()) {
                auto* target = resolveSymlink(*static_cast<const Symlink*>(file));
                if(!target) return nullptr;
                if(!target->isDirectory()) return nullptr;
                dir = static_cast<Directory*>(target);
                continue;
            }
            // Then try to get it on the host (if possible)
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

    File* FS::resolveSymlink(const Symlink& symlink, u32 maxLinks) {
        verify(maxLinks == 0, "Add support for recursive symlink resolution");
        const std::string& targetPathname = symlink.link();
        if(targetPathname.empty()) return nullptr;
        verify(targetPathname[0] == '/', "Only absolute symlinks can be resolved");
        auto targetPath = Path::tryCreate(targetPathname);
        verify(!!targetPath, "Unable to create symlink's target path");
        return tryGetFile(*targetPath, FollowSymlink::YES);
    }

    File* FS::tryGetFile(const Path& path, FollowSymlink followSymlink) {
        Directory* dir = root();
        if(path.isRoot()) return dir;
        for(const std::string& component : path.componentsExceptLast()) {
            Directory* subdir = dir->tryGetSubDirectory(component);
            if(!!subdir) {
                dir = subdir;
                continue;
            } else {
                // maybe we have an intermediate symlink
                File* file = dir->tryGetEntry(component);
                if(!!file && file->isSymlink()) {
                    auto* target = resolveSymlink(*static_cast<const Symlink*>(file));
                    if(!target) return nullptr;
                    if(!target->isDirectory()) return nullptr;
                    dir = static_cast<Directory*>(target);
                    continue;
                }
            }
            return nullptr;
        }
        File* file = dir->tryGetEntry(path.last());
        if(!!file && file->isSymlink() && followSymlink == FollowSymlink::YES) {
            return resolveSymlink(*static_cast<const Symlink*>(file));
        }
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

    FS::FD FS::open(FD dirfd,
            const std::string& pathname,
            BitFlags<FS::AccessMode> accessMode,
            BitFlags<FS::CreationFlags> creationFlags,
            BitFlags<FS::StatusFlags> statusFlags,
            Permissions permissions) {
        (void)permissions;
        if(pathname.empty()) return FD{-ENOENT};

        // For some reason, 64-bit linux adds this flag without notification
        statusFlags.add(StatusFlags::LARGEFILE);

        bool canUseHostFile = true;
        if(accessMode.test(AccessMode::WRITE))       canUseHostFile = false;
        if(creationFlags.test(CreationFlags::CREAT)) canUseHostFile = false;
        if(creationFlags.test(CreationFlags::TRUNC)) canUseHostFile = false;
        if(statusFlags.test(StatusFlags::APPEND))    canUseHostFile = false;

        // Look if the file is already present in FS, open or closed.
        auto absolutePathname = toAbsolutePathname(pathname, dirfd);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");

        bool closeOnExec = creationFlags.test(CreationFlags::CLOEXEC);

        auto openNode = [&](File* filePtr) -> FD {
            FD fd = allocateFd();
            openFileDescriptions_.push_back(OpenFileDescription(filePtr, accessMode, statusFlags));
            openFiles_.push_back(OpenNode{fd, &openFileDescriptions_.back(), closeOnExec});
            filePtr->ref();
            return fd;
        };
        
        File* file = tryGetFile(*path, FollowSymlink::YES);
        if(!!file) {
            FD fd = openNode(file);
            file->open();
            return fd;
        }

        // Otherwise, try to create it
        if(canUseHostFile) {
            if(creationFlags.test(CreationFlags::DIRECTORY)) {
                // open the directory
                auto hostBackedDirectory = HostDirectory::tryCreate(this, root_.get(), absolutePathname);
                if(!hostBackedDirectory) {
                    // TODO: return the actual value of errno
                    return FS::FD{-ENOENT};
                }
                
                // create and add the node to the filesystem
                hostBackedDirectory->open();
                return insertNode(std::move(hostBackedDirectory), accessMode, statusFlags, closeOnExec);
            } else {
                // try open the directory
                auto hostBackedDirectory = HostDirectory::tryCreate(this, root_.get(), absolutePathname);
                if(!!hostBackedDirectory) {
                    // create and add the node to the filesystem
                    hostBackedDirectory->open();
                    return insertNode(std::move(hostBackedDirectory), accessMode, statusFlags, closeOnExec);
                }

                // try open the file
                auto* hostBackedFile = HostFile::tryCreateAndAdd(this, root_.get(), absolutePathname, accessMode, closeOnExec);
                if(!!hostBackedFile) {
                    // create and add the node to the filesystem
                    hostBackedFile->open();
                    return openNode(hostBackedFile);
                }

                // try open device
                auto* hostDevice = HostDevice::tryCreateAndAdd(this, root_.get(), absolutePathname);
                if(!!hostDevice) {
                    // create and add the node to the filesystem
                    hostDevice->open();
                    return openNode(hostDevice);
                }

                // TODO: return the actual value of errno
                return FS::FD{-ENOENT};
            }
        } else {
            // open the file
            bool createIfNotFound = creationFlags.test(CreationFlags::CREAT);
            auto* shadowFile = ShadowFile::tryCreateAndAdd(this, root_.get(), absolutePathname, createIfNotFound);
            if(!!shadowFile) {
                if(creationFlags.test(CreationFlags::TRUNC)) shadowFile->truncate(0);
                shadowFile->setWritable(accessMode.test(AccessMode::WRITE));
                
                // create and add the node to the filesystem
                shadowFile->open();
                return openNode(shadowFile);
            }

            // try open device
            auto* shadowDevice = ShadowDevice::tryCreateAndAdd(this, root_.get(), absolutePathname);
            if(!!shadowDevice) {
                // create and add the node to the filesystem
                shadowDevice->open();
                return openNode(shadowDevice);
            }

            // TODO: return the actual value of errno
            return FS::FD{-ENOENT};
        }
        return FD{-EINVAL};
    }

    FS::FD FS::dup(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return FD{-EBADF};
        FD newFd = allocateFd();
        openFileDescription->file()->ref();
        openFiles_.push_back(OpenNode{newFd, openFileDescription, false});
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
        openFiles_.push_back(OpenNode{newfd, oldOfd, false});
        return newfd;
    }

    FS::FD FS::dup3(FS::FD oldfd, FS::FD newfd, int flags) {
        if(oldfd == newfd) return FD{-EINVAL};
        OpenFileDescription* oldOfd = findOpenFileDescription(oldfd);
        if(!oldOfd) return FD{-EBADF};
        OpenFileDescription* newOfd = findOpenFileDescription(newfd);
        if(!!newOfd) {
            int ret = close(newfd);
            verify(ret == 0, "close in dup3 failed");
        }
        oldOfd->file()->ref();
        bool closeOnExec = Host::Open::isCloseOnExec(flags);
        openFiles_.push_back(OpenNode{newfd, oldOfd, closeOnExec});
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
        File* filePtr = tryGetFile(*path, FollowSymlink::YES);
        if(!filePtr) return -ENOENT;
        if(filePtr->refCount() > 0) {
            filePtr->setDeleteAfterClose();
        } else {
            auto file = tryTakeFile(*path);
            // let the file die  here
        }
        return 0;
    }

    ErrnoOrBuffer FS::readlink(const std::string& pathname, size_t bufferSize) {
        if(pathname.empty()) return ErrnoOrBuffer(-ENOENT);
        auto absolutePathname = toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        File* file = tryGetFile(*path, FollowSymlink::NO);
        if(!!file) {
            if(!file->isSymlink()) return ErrnoOrBuffer(-EINVAL);
            Symlink* symlink = static_cast<Symlink*>(file);
            return symlink->readlink(bufferSize);
        } else {
            auto* symlink = HostSymlink::tryCreateAndAdd(this, root_.get(), absolutePathname);
            if(!symlink) return ErrnoOrBuffer(-EINVAL);
            verify(symlink->isSymlink());
            return static_cast<Symlink*>(symlink)->readlink(bufferSize);
        }
    }

    int FS::access(const std::string& pathname, int mode) const {
        auto absolutePathname = toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        return Host::access(absolutePathname, mode);
    }

    int FS::faccessat(FS::FD dirfd, const std::string& pathname, int mode) const {
        auto absolutePathname = toAbsolutePathname(pathname, dirfd);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        return Host::access(absolutePathname, mode);
    }

    FS::FD FS::memfd_create(const std::string& name, unsigned int flags) {
        verify(!Host::MemfdFlags::isOther(flags), "Allow (and ignore) cloexec and allow_sealing");
        auto shadowFile = ShadowFile::tryCreate(this, name);
        if(!shadowFile) return FS::FD{-ENOMEM};
        shadowFile->setDeleteAfterClose();
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        return insertNode(std::move(shadowFile), accessMode, statusFlags, Host::MemfdFlags::isCloseOnExec(flags));
    }

    FS::OpenNode* FS::findOpenNode(FD fd) {
        for(OpenNode& node : openFiles_) {
            if(node.fd != fd) continue;
            return &node;
        }
        return nullptr;
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
        File* file = openFileDescription->file();
        if(!file->isReadable()) ErrnoOrBuffer{-EBADF};
        return openFileDescription->read(count);
    }

    ErrnoOrBuffer FS::pread(FD fd, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer{-EBADF};
        File* file = openFileDescription->file();
        if(!file->isReadable()) ErrnoOrBuffer{-EBADF};
        return openFileDescription->pread(count, offset);
    }

    ssize_t FS::readv(FD fd, std::vector<Buffer>* buffers) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isReadable()) return -EBADF;
        ssize_t nbytes = 0;
        for(Buffer& buf : *buffers) {
            ErrnoOrBuffer errnoOrReadBuffer = openFileDescription->read(buf.size());
            ssize_t ret = errnoOrReadBuffer.errorOrWith<ssize_t>([&](const Buffer& readBuffer) {
                verify(readBuffer.size() <= buf.size());
                ::memcpy(buf.data(), readBuffer.data(), readBuffer.size());
                return (ssize_t)readBuffer.size();
            });
            if(ret < 0) return ret;
            nbytes += ret;
        }
        return nbytes;
    }

    ssize_t FS::write(FD fd, const u8* buf, size_t count) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        if(!file->isWritable()) return -EBADF;
        return openFileDescription->write(buf, count);
    }

    ssize_t FS::pwrite(FD fd, const u8* buf, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        if(!file->isWritable()) return -EBADF;
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
        auto absolutePathname = toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        if(!!path) {
            File* file = tryGetFile(*path, FollowSymlink::YES);
            if(!!file) return file->stat();
        }
        return Host::stat(pathname);
    }

    ErrnoOrBuffer FS::fstat(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->stat();
    }

    ErrnoOrBuffer FS::statx(FD dirfd, const std::string& pathname, int flags, unsigned int mask) { // NOLINT(readability-convert-member-functions-to-static)
        // statx() uses pathname, dirfd, and flags to identify the target file in one of the following ways:
        if(pathname.empty()) {
            if(Host::Statx::isAtEmptyPath(flags)) {
                // By file descriptor
                // If  pathname  is  an  empty string and the AT_EMPTY_PATH flag is specified in flags (see below), then the target file is the one re‐
                // ferred to by the file descriptor dirfd.
                OpenFileDescription* openFileDescription = findOpenFileDescription(dirfd);
                if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
                return openFileDescription->file()->statx(mask);
            } else {
                return ErrnoOrBuffer(-ENOENT);
            }
        } else if(pathname[0] == '/') {
            // An absolute pathname
            // If pathname begins with a slash, then it is an absolute pathname that identifies the target file. In this case, dirfd is ignored.
            auto path = Path::tryCreate(pathname);
            verify(!!path, "Unable to create path");
            FollowSymlink followSymlink = Host::Fstatat::isSymlinkNofollow(flags) ? FollowSymlink::NO : FollowSymlink::YES;
            File* file = tryGetFile(*path, followSymlink);
            if(!!file) {
                return file->statx(mask);
            } else {
                // if we don't know the file, delegating to the host is probably fine
                return Host::statx(Host::cwdfd(), pathname, flags, mask);
            }
        } else {
            if(dirfd.fd == Host::cwdfd().fd) {
                // A relative pathname
                // If pathname is a string that begins with a character other than a slash and dirfd is AT_FDCWD, then pathname is a relative pathname
                // that is interpreted relative to the process's current working directory.
                verify(!!cwd());
                auto path = Path::tryJoin(cwd()->path(), pathname);
                verify(!!path, "Unable to create path");
                FollowSymlink followSymlink = Host::Fstatat::isSymlinkNofollow(flags) ? FollowSymlink::YES : FollowSymlink::NO;
                File* file = tryGetFile(*path, followSymlink);
                if(!!file) {
                    return file->statx(mask);
                } else {
                    // if we don't know the file, delegating to the host is probably fine
                    return Host::statx(Host::cwdfd(), pathname, flags, mask);
                }
            } else {
                // A directory-relative pathname
                // If pathname is a string that begins with a character other than a slash and dirfd is a file descriptor that refers to a directory,
                // then pathname is a relative pathname that is interpreted relative to the directory referred to by dirfd.
                OpenFileDescription* dirOpenFileDescription = findOpenFileDescription(dirfd);
                verify(!!dirOpenFileDescription, "trying to perform relative path lookup in statx w.r.t. unopened directory");
                if(!dirOpenFileDescription->file()->isDirectory()) return ErrnoOrBuffer(-ENOTDIR);
                verify(false, "implement directory-based relative path lookup in statx");
            }

        }
        verify(false, "Unexpected code path in FS::statx");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer FS::fstatat64(FD dirfd, const std::string& pathname, int flags) {
        if(Host::Fstatat::isEmptyPath(flags)) {
            return fstat(dirfd);
        }
        Directory* dir = cwd();
        if(dirfd.fd != Host::cwdfd().fd) {
            OpenFileDescription* ofd = findOpenFileDescription(dirfd);
            if(!ofd) return ErrnoOrBuffer(-EBADF);
            File* file = ofd->file();
            if(!file) return ErrnoOrBuffer(-EBADF);
            if(!file->isDirectory()) return ErrnoOrBuffer(-ENOTDIR);
            dir = static_cast<Directory*>(file);
        }
        auto path = Path::tryJoin(dir->path(), pathname);
        if(!!path) {
            verify(!Host::Fstatat::isNoAutomount(flags), "no automount not supported");
            FollowSymlink followSymlink = Host::Fstatat::isSymlinkNofollow(flags) ? FollowSymlink::YES : FollowSymlink::NO;
            File* file = tryGetFile(*path, followSymlink);
            if(!file) return ErrnoOrBuffer(-ENOENT);
            return file->stat();
        } else {
            return Host::fstatat64(Host::cwdfd(), pathname, flags);
        }
    }

    ErrnoOrBuffer FS::fstatfs(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->statfs();
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
        openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& openNode) {
            return openNode.fd == fd;
        }), openFiles_.end());
        file->unref();
        if(file->refCount() == 0) {
            file->close();
            if(!file->keepAfterClose() || file->deleteAfterClose()) {
                unlink(file->path());
            }
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
        std::optional<int> emulatedRet;
        // If we can do it in FS alone, do it here
        // and check if we need to do it on the file as well
        bool callFcntlOnFile = true;
        switch(Host::Fcntl::toCommand(cmd)) {
            case Host::Fcntl::DUPFD: {
                FD newfd = dup(fd);
                emulatedRet = newfd.fd;
                callFcntlOnFile = false;
                break;
            }
            case Host::Fcntl::DUPFD_CLOEXEC: {
                FD newfd = dup(fd);
                OpenNode* newNode = findOpenNode(newfd);
                verify(!!newNode);
                newNode->closeOnExec = true;
                emulatedRet = newfd.fd;
                callFcntlOnFile = false;
                break;
            }
            case Host::Fcntl::GETFD: {
                OpenNode* node = findOpenNode(fd);
                verify(!!node);
                emulatedRet = node->closeOnExec ? Host::Fcntl::fdCloExec() : 0;
                break;
            }
            case Host::Fcntl::SETFD: {
                OpenNode* node = findOpenNode(fd);
                verify(!!node);
                bool currentFlag = node->closeOnExec;
                if(Host::Open::isCloseOnExec(arg) && !currentFlag) node->closeOnExec = true;
                if(!Host::Open::isCloseOnExec(arg) && currentFlag) node->closeOnExec = false;
                emulatedRet = 0;
                break;
            }
        // If the open file description is sufficient, do it there
            case Host::Fcntl::GETFL: {
                emulatedRet = assembleAccessModeAndFileStatusFlags(openFileDescription->accessMode(), openFileDescription->statusFlags());
                callFcntlOnFile = false;
                break;
            }
            case Host::Fcntl::SETFL: {
                verify(!Host::Open::isAppending(arg), "changing append flag is not supported");
                auto currentFlags = openFileDescription->statusFlags();
                if(Host::Open::isNonBlock(arg) && !currentFlags.test(StatusFlags::NONBLOCK)) {
                    openFileDescription->statusFlags().add(StatusFlags::NONBLOCK);
                }
                if(!Host::Open::isNonBlock(arg) && currentFlags.test(StatusFlags::NONBLOCK)) {
                    openFileDescription->statusFlags().remove(StatusFlags::NONBLOCK);
                }
                emulatedRet = 0;
                break;
            }
            default: break;
        }
        // Otherwise, go ask the file.
        // Note: we may have to go here, so the information makes it to the host when relevant
        std::optional<int> fileRet;
        if(callFcntlOnFile) {
            File* file = openFileDescription->file();
            fileRet = file->fcntl(cmd, arg);
        }
        if(!!emulatedRet) {
            if(!!fileRet && emulatedRet != fileRet) {
                warn(fmt::format("Emulation of fcntl failed : emulated = {}  file = {}\n"
                                 "    Note: this may be due to O_LARGEFILE (32768)",
                    emulatedRet.value(), fileRet.value()));
                verify(false);
            }
            return emulatedRet.value();
        } else {
            verify(!!fileRet);
            return fileRet.value();
        }
    }

    ErrnoOrBuffer FS::ioctl(FD fd, Ioctl request, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->ioctl(request, buffer);
    }

    int FS::flock(FD fd, int operation) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        bool lockShared = Host::Lock::isShared(operation);
        bool lockExclusively = Host::Lock::isExclusive(operation);
        bool unlock = Host::Lock::isUnlock(operation);
        bool nonBlocking = Host::Lock::isNonBlocking(operation);
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

    int FS::fallocate(FD fd, int mode, off_t offset, off_t len) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        verify(file->isRegularFile(), "fallocate not implemented on non-regular files");
        verify(file->isShadow(), "fallocate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        return shadowFile->fallocate(mode, offset, len);
    }

    int FS::truncate(const std::string& pathname, off_t length) {
        auto path = Path::tryCreate(pathname);
        verify(!!path, "Unable to build path");
        File* file = tryGetFile(*path, FollowSymlink::YES);
        if(!file) return -ENOENT;
        verify(file->isRegularFile(), "truncate not implemented on non-regular files");
        verify(file->isShadow(), "truncate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        shadowFile->truncate((size_t)length);
        return 0;
    }

    int FS::ftruncate(FD fd, off_t length) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        verify(file->isRegularFile(), "fallocate not implemented on non-regular files");
        verify(file->isShadow(), "fallocate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        shadowFile->truncate((size_t)length);
        return 0;
    }

    FS::FD FS::eventfd2(unsigned int initval, int flags) {
        verify(!Host::Eventfd2Flags::isSemaphore(flags), "only closeOnExec and nonBlock are allowed on eventfd2");
        std::unique_ptr<Event> event = Event::tryCreate(this, initval, flags);
        verify(!!event, "Unable to create event");
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        if(Host::Eventfd2Flags::isNonBlock(flags)) statusFlags.add(StatusFlags::NONBLOCK);
        bool closeOnExec = Host::Eventfd2Flags::isCloseOnExec(flags);
        FD fd = insertNode(std::move(event), accessMode, statusFlags, closeOnExec);
        return fd;
    }

    FS::FD FS::epoll_create1(int flags) {
        std::unique_ptr<Epoll> epoll = std::make_unique<Epoll>(this, flags);
        verify(!!epoll, "Unable to create epoll");
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        bool closeOnExec = Host::EpollFlags::isCloseOnExec(flags);
        return insertNode(std::move(epoll), accessMode, statusFlags, closeOnExec);
    }

    int FS::epoll_ctl(FD epfd, int op, FD fd, BitFlags<EpollEventType> events, u64 data) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(epfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isEpoll()) return -EBADF;
        Epoll* epoll = static_cast<Epoll*>(openFileDescription->file());
        if(Host::EpollCtlOp::isAdd(op)) {
            events.add(EpollEventType::HANGUP);
            auto ret = epoll->addEntry(fd.fd, events.toUnderlying(), data);
            return ret.errorOr(0);
        } else if(Host::EpollCtlOp::isMod(op)) {
            auto ret = epoll->changeEntry(fd.fd, events.toUnderlying(), data);
            return ret.errorOr(0);
        } else {
            verify(Host::EpollCtlOp::isDel(op), "Unknown epoll_ctl op");
            auto ret = epoll->deleteEntry(fd.fd);
            return ret.errorOr(0);
        }
    }

    int FS::epollWaitImmediate(FD epfd, std::vector<EpollEvent>* events) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(epfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isEpoll()) return -EBADF;
        doEpollWait(epfd, events);
        return (int)events->size();
    }

    void FS::doEpollWait(FD epfd, std::vector<EpollEvent>* events) {
        verify(!!events);
        OpenFileDescription* openFileDescription = findOpenFileDescription(epfd);
        verify(!!openFileDescription, "No open file description for epoll wait");
        verify(openFileDescription->file()->isEpoll(), "Cannot perform epoll wait on non-epoll instance");
        Epoll* instance = static_cast<Epoll*>(openFileDescription->file());
        instance->forEachEntryInInterestList([&](i32 fd, u32 ev, u64 data) {
            OpenFileDescription* openFileDescription = findOpenFileDescription(FD{fd});
            if(!openFileDescription) {
                events->push_back(EpollEvent{
                    BitFlags<EpollEventType>{EpollEventType::HANGUP},
                    0,
                });
                return;
            }
            File* file = openFileDescription->file();
            verify(file->isPollable(), [&]() { fmt::print("fd={} is not pollable\n", fd); });

            BitFlags<EpollEventType> eventTypes = BitFlags<EpollEventType>::fromIntegerType(ev);
            bool testRead = eventTypes.test(EpollEventType::CAN_READ);
            bool testWrite = eventTypes.test(EpollEventType::CAN_WRITE);

            BitFlags<EpollEventType> untestedFlags = eventTypes;
            untestedFlags.remove(EpollEventType::CAN_READ);
            untestedFlags.remove(EpollEventType::CAN_WRITE);
            untestedFlags.remove(EpollEventType::HANGUP);
            verify(!untestedFlags.any(), "Unexpected event in epoll-wait");

            BitFlags<EpollEventType> outputEvents;
            if(testRead && file->canRead())   outputEvents.add(EpollEventType::CAN_READ);
            if(testWrite && file->canWrite()) outputEvents.add(EpollEventType::CAN_WRITE);

            events->push_back(EpollEvent{
                outputEvents,
                data,
            });
        });
    }

    FS::FD FS::socket(int domain, int type, int protocol) {
        auto socket = Socket::tryCreate(this, domain, type, protocol);
        // TODO: return the actual value of errno
        if(!socket) return FS::FD{-EINVAL};
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        if(Host::SocketType::isNonBlock(type)) statusFlags.add(StatusFlags::NONBLOCK);
        bool closeOnExec = Host::SocketType::isCloseOnExec(type);
        return insertNode(std::move(socket), accessMode, statusFlags, closeOnExec);
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

    ErrnoOrBuffer FS::getsockopt(FD sockfd, int level, int optname, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getsockopt(level, optname, buffer);
    }

    int FS::setsockopt(FD sockfd, int level, int optname, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->setsockopt(level, optname, buffer);
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

            bool testRead = ((rfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ);
            bool testWrite = ((rfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE);
            if(testRead && file->canRead())   rfd.revents = rfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) rfd.revents = rfd.revents | PollEvent::CAN_WRITE;
        }
        int ret = 0;
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
            verify(file->isPollable(), [&]() { fmt::print("Attempting to poll non-pollable file with fd={}\n", fd.fd); });
            bool testRead = (pollfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ;
            bool testWrite = (pollfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE;
            if(testRead && file->canRead())   pollfd.revents = pollfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) pollfd.revents = pollfd.revents | PollEvent::CAN_WRITE;
        }
    }

    int FS::selectImmediate(SelectData* selectData) {
        if(!selectData) return 0;
        for(size_t fd = 0; fd < (size_t)selectData->nfds; ++fd) {
            bool testRead = selectData->readfds.test(fd);
            bool testWrite = selectData->writefds.test(fd);
            if(!testRead && !testWrite) continue;

            selectData->readfds.reset(fd);
            selectData->writefds.reset(fd);

            // check that all fds are pollable and have a host-side fd
            OpenFileDescription* openFileDescription = findOpenFileDescription(FD{(i32)fd});
            if(!openFileDescription) return -EBADF;

            File* file = openFileDescription->file();
            verify(file->isPollable(), [&]() { fmt::print("fd={} is not pollable\n", fd); });

            if(testRead && file->canRead())   selectData->readfds.set(fd);
            if(testWrite && file->canWrite()) selectData->writefds.set(fd);
        }
        return 0;
    }

    ErrnoOr<std::pair<FS::FD, FS::FD>> FS::pipe2(int flags) {
        using ReturnType = ErrnoOr<std::pair<FS::FD, FS::FD>>;
        auto pipe = Pipe::tryCreate(this, flags);
        if(!pipe) return ReturnType(-EMFILE);

        auto readingSide = pipe->tryCreateReader();
        if(!readingSide) return ReturnType(-EMFILE);

        auto writingSide = pipe->tryCreateWriter();
        if(!writingSide) return ReturnType(-EMFILE);

        BitFlags<CreationFlags> creationFlags = FS::toCreationFlags(flags);
        bool closeOnExec = creationFlags.test(CreationFlags::CLOEXEC);

        pipes_.push_back(std::move(pipe));
        BitFlags<StatusFlags> statusFlags = FS::toStatusFlags(flags);
        verify(!statusFlags.test(StatusFlags::DIRECT), "O_DIRECT not supported on pipes");
        FD reader = insertNode(std::move(readingSide), BitFlags<AccessMode>{AccessMode::READ}, statusFlags, closeOnExec);
        FD writer = insertNode(std::move(writingSide), BitFlags<AccessMode>{AccessMode::WRITE}, statusFlags, closeOnExec);
        return ReturnType(std::make_pair(reader, writer));
    }

    ErrnoOr<std::pair<Buffer, Buffer>> FS::recvfrom(FD sockfd, size_t len, int flags, bool requireSrcAddress) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->recvfrom(len, flags, requireSrcAddress);

    }
    ssize_t FS::recvmsg(FD sockfd, int flags, Message* message) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        Socket::Message socketMessage;
        socketMessage.msg_name = message->msg_name;
        socketMessage.msg_iov = message->msg_iov;
        socketMessage.msg_control = message->msg_control;
        socketMessage.msg_flags = message->msg_flags;
        ssize_t ret = socket->recvmsg(flags, &socketMessage);
        message->msg_name = socketMessage.msg_name;
        message->msg_iov = socketMessage.msg_iov;
        message->msg_control = socketMessage.msg_control;
        message->msg_flags = socketMessage.msg_flags;
        return ret;
    }

    ssize_t FS::send(FD sockfd, const Buffer& buffer, int flags) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -ENOTSOCK;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->send(buffer, flags);
    }

    ssize_t FS::sendmsg(FD sockfd, int flags, const Message& message) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(sockfd);
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        Socket::Message socketMessage;
        socketMessage.msg_name = message.msg_name;
        socketMessage.msg_iov = message.msg_iov;
        socketMessage.msg_control = message.msg_control;
        socketMessage.msg_flags = message.msg_flags;
        return socket->sendmsg(flags, socketMessage);
    }

    std::string FS::filename(FD fd) {
        OpenFileDescription* openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return "Unknown";
        File* file = openFileDescription->file();
        return file->path();
    }

    void FS::dumpSummary() const {
        fmt::print("Open files:\n");
        for(const auto& openFile : openFiles_) {
            fmt::print("  fd={} : type={:20} path={}\n", openFile.fd.fd, openFile.openFiledescription->toString(), openFile.openFiledescription->file()->path());
        }
        root_->printSubtree();
    }

    int FS::assembleAccessModeAndFileStatusFlags(BitFlags<AccessMode> accessMode, BitFlags<StatusFlags> statusFlags) {
        int ret = 0;
        bool read = accessMode.test(AccessMode::READ);
        bool write = accessMode.test(AccessMode::WRITE);
        assert(read | write); // cannot have none
        if(read && write) {
            ret = 2;
        } else if(write) {
            ret = 1;
        } else {
            ret = 0;
        }
        auto getActiveBit = [](bool(*func)(int)) -> int {
            for(int i = 0; i < 32; ++i) {
                int mask = 1 << i;
                if(func(mask)) return mask;
            }
            return 0;
        };
        // Quick and dirty: do it by asking the host for each bit
        // see toStatusFlags
        if(statusFlags.test(StatusFlags::APPEND))    ret |= getActiveBit(&Host::Open::isAppending);
        if(statusFlags.test(StatusFlags::LARGEFILE)) ret |= getActiveBit(&Host::Open::isLargeFile);
        if(statusFlags.test(StatusFlags::NONBLOCK))  ret |= getActiveBit(&Host::Open::isNonBlock);
        return ret;
    }

}