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

    CurrentDirectoryOrDirectoryDescriptor FileDescriptors::dirfd(FD dirfd, Directory* cwd) {
        bool isCwd = dirfd.fd == Host::cwdfd().fd;
        if(isCwd) {
            return CurrentDirectoryOrDirectoryDescriptor {
                true,
                cwd,
                FileDescriptor{},
            };
        } else {
            auto* descriptor = findFileDescriptor(dirfd);
            return CurrentDirectoryOrDirectoryDescriptor {
                false,
                nullptr,
                (!!descriptor) ? *descriptor : FileDescriptor{},
            };   
        }
    }

    FD FileDescriptors::open(const Path& path,
            BitFlags<AccessMode> accessMode,
            BitFlags<CreationFlags> creationFlags,
            BitFlags<StatusFlags> statusFlags,
            Permissions permissions) {
        auto descriptor = fs_.open(path, accessMode, creationFlags, statusFlags, permissions);
        int ret = descriptor.errorOrWith<int>([&](auto& descr) {
            FD fd = allocateFd();
            fileDescriptors_[fd.fd] = std::make_unique<FileDescriptor>(descr);
            return fd.fd;
        });
        return FD{ret};
    }

    int FileDescriptors::close(FD fd) {
        auto* descriptor = findFileDescriptor(fd);
        if(!descriptor) return -EBADF;
        int ret = fs_.close(*descriptor);
        if(ret < 0) return ret;
        fileDescriptors_[fd.fd] = nullptr;
        return ret;
    }

    int FileDescriptors::fcntl(FD fd, int cmd, int arg) {
        auto descriptor = findFileDescriptor(fd);
        if(!descriptor) return -EBADF;
        OpenFileDescription* openFileDescription = descriptor->openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        switch(Host::Fcntl::toCommand(cmd)) {
            case Host::Fcntl::DUPFD: {
                verify("cannot do dupfd in FS::fcntl");
                FD newfd = dup(fd);
                return newfd.fd;
            }
            case Host::Fcntl::DUPFD_CLOEXEC: {
                verify("cannot do dupfd_cloexec in FS::fcntl");
                FD newfd = dup(fd);
                FileDescriptor* desc = findFileDescriptor(newfd);
                verify(!!desc);
                desc->closeOnExec = true;
                return newfd.fd;
            }
            default: {
                return fs_.fcntl(*descriptor, cmd, arg);
            }
        }
    }

    FD FileDescriptors::dup(FD fd) {
        auto openFileDescription = findOpenFileDescription(fd);
        if(!openFileDescription) return FD{-EBADF};
        FD newFd = allocateFd();
        openFileDescription->file()->ref();
        FileDescriptor descriptor { openFileDescription, false };
        fileDescriptors_[newFd.fd] = std::make_unique<FileDescriptor>(descriptor);
        return newFd;
    }

    FD FileDescriptors::dup2(FD oldfd, FD newfd) {
        auto oldOfd = findOpenFileDescription(oldfd);
        if(!oldOfd) return FD{-EBADF};
        if(oldfd == newfd) return newfd;
        auto newOfd = findOpenFileDescription(newfd);
        if(!!newOfd) {
            int ret = close(newfd);
            verify(ret == 0, "close in dup2 failed");
        }
        oldOfd->file()->ref();
        if(newfd.fd >= (int)fileDescriptors_.size()) {
            fileDescriptors_.resize(newfd.fd+1);
        }
        FileDescriptor descriptor { oldOfd, false };
        fileDescriptors_[newfd.fd] = std::make_unique<FileDescriptor>(descriptor);
        return newfd;
    }

    FD FileDescriptors::dup3(FD oldfd, FD newfd, int flags) {
        if(oldfd == newfd) return FD{-EINVAL};
        auto oldOfd = findOpenFileDescription(oldfd);
        if(!oldOfd) return FD{-EBADF};
        auto newOfd = findOpenFileDescription(newfd);
        if(!!newOfd) {
            int ret = close(newfd);
            verify(ret == 0, "close in dup3 failed");
        }
        oldOfd->file()->ref();
        bool closeOnExec = Host::Open::isCloseOnExec(flags);
        if(newfd.fd >= (int)fileDescriptors_.size()) {
            fileDescriptors_.resize(newfd.fd+1);
        }
        FileDescriptor descriptor { oldOfd, closeOnExec };
        fileDescriptors_[newfd.fd] = std::make_unique<FileDescriptor>(descriptor);
        return newfd;
    }

    FD FileDescriptors::memfd_create(const std::string& name, unsigned int flags) {
        auto descriptor = fs_.memfd_create(name, flags);
        int ret = descriptor.errorOrWith<int>([&](auto& descr) {
            FD fd = allocateFd();
            fileDescriptors_[fd.fd] = std::make_unique<FileDescriptor>(descr);
            return fd.fd;
        });
        return FD{ret};
    }

    FD FileDescriptors::eventfd2(unsigned int initval, int flags) {
        auto descriptor = fs_.eventfd2(initval, flags);
        int ret = descriptor.errorOrWith<int>([&](auto& descr) {
            FD fd = allocateFd();
            fileDescriptors_[fd.fd] = std::make_unique<FileDescriptor>(descr);
            return fd.fd;
        });
        return FD{ret};
    }

    FD FileDescriptors::epoll_create1(int flags) {
        auto descriptor = fs_.epoll_create1(flags);
        int ret = descriptor.errorOrWith<int>([&](auto& descr) {
            FD fd = allocateFd();
            fileDescriptors_[fd.fd] = std::make_unique<FileDescriptor>(descr);
            return fd.fd;
        });
        return FD{ret};
    }

    FD FileDescriptors::socket(int domain, int type, int protocol) {
        auto descriptor = fs_.socket(domain, type, protocol);
        int ret = descriptor.errorOrWith<int>([&](auto& descr) {
            FD fd = allocateFd();
            fileDescriptors_[fd.fd] = std::make_unique<FileDescriptor>(descr);
            return fd.fd;
        });
        return FD{ret};
    }

    ErrnoOr<std::pair<FD, FD>> FileDescriptors::pipe2(int flags) {
        auto descriptors = fs_.pipe2(flags);
        auto ret = descriptors.transform<std::pair<FD, FD>>([&](const std::pair<FileDescriptor, FileDescriptor>& fds) -> std::pair<FD, FD> {
            FileDescriptor reader = fds.first;
            FileDescriptor writer = fds.second;
            FD readerFd = allocateFd();
            fileDescriptors_[readerFd.fd] = std::make_unique<FileDescriptor>(reader);
            FD writerFd = allocateFd();
            fileDescriptors_[writerFd.fd] = std::make_unique<FileDescriptor>(writer);
            return std::make_pair(readerFd, writerFd);
        });
        return ret;
    }


    void FileDescriptors::createStandardStreams(const Path& ttypath) {
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<CreationFlags> creationFlags {};
        BitFlags<StatusFlags> statusFlags {};
        Permissions permissions;
        permissions.userReadable = true;
        permissions.userWriteable = true;
        auto stdinFd = open(ttypath, accessMode, creationFlags, statusFlags, permissions);
        auto stdoutFd = open(ttypath, accessMode, creationFlags, statusFlags, permissions);
        auto stderrFd = open(ttypath, accessMode, creationFlags, statusFlags, permissions);
        verify(stdinFd.fd == 0, "stdin must have fd 0");
        verify(stdoutFd.fd == 1, "stdout must have fd 1");
        verify(stderrFd.fd == 2, "stderr must have fd 2");
    }

    std::unique_ptr<FileDescriptors> FileDescriptors::clone() {
        auto fds = std::make_unique<FileDescriptors>(fs_);
        fds->fileDescriptors_.resize(fileDescriptors_.size());
        for(int i = 0; i < (int)fileDescriptors_.size(); ++i) {
            if(!fileDescriptors_[i]) continue;
            FileDescriptor descriptor = *fileDescriptors_[i];
            descriptor.openFiledescription->file()->ref();
            fds->fileDescriptors_[i] = std::make_unique<FileDescriptor>(descriptor);
        }
        return fds;
    }

    FD FileDescriptors::allocateFd() {
        for(int i = 0; i < (int)fileDescriptors_.size(); ++i) {
            if(!fileDescriptors_[i]) return FD{i};
        }
        FD fd{(int)fileDescriptors_.size()};
        fileDescriptors_.emplace_back(nullptr);
        return fd;
    }

    FileDescriptor* FileDescriptors::findFileDescriptor(FD fd) {
        if(fd.fd < 0) return nullptr;
        if(fd.fd >= (int)fileDescriptors_.size()) return nullptr;
        return fileDescriptors_[fd.fd].get();
    }

    std::shared_ptr<OpenFileDescription> FileDescriptors::findOpenFileDescription(FD fd) {
        if(fd.fd < 0) return nullptr;
        if(fd.fd >= (int)fileDescriptors_.size()) return nullptr;
        if(!fileDescriptors_[fd.fd]) return nullptr;
        return fileDescriptors_[fd.fd]->openFiledescription;
    }

    void FileDescriptors::dumpSummary() const {
        fmt::println("File descriptors:");
        for(size_t fd = 0; fd < fileDescriptors_.size(); ++fd) {
            auto* desc = fileDescriptors_[fd].get();
            if(!desc) continue;
            auto* ofd = desc->openFiledescription.get();
            fmt::print("  fd={} : type={:20} path={}\n", fd, ofd->toString(), ofd->file()->path().absolute());
        }
    }


    FS::FS() {
        root_ = HostDirectory::tryCreateRoot();
        verify(!!root_, "Unable to create root directory");
        {
            auto ttyname = Host::ttyname().value_or("/dev/tty");
            auto ttypath = Path::tryCreate(ttyname);
            verify(!!ttypath, "Unable to create tty path");
            auto tty = Tty::tryCreate(*ttypath, false);
            verify(!!tty, "Unable to create tty");
            auto* dir = ensurePathExceptLast(*ttypath);
            tty_ = dir->addFile(std::move(tty));
        }
    }

    Directory* FS::findCurrentWorkDirectory(const Path& cwd) {
        return ensureCompletePath(cwd);
    }

    Path FS::ttyPath() const { return tty_->path(); }

    void FS::resetProcFS(int pid, const Path& programFilePath) {
        // create a proc fs
        Path proc("proc");
        auto procfs = ProcFS::tryCreate(proc);
        Directory* procParent = ensurePathExceptLast(proc);
        procfs_ = procParent->addFile(std::move(procfs));
        verify(!!procfs_, "Unable to create procFS");

        // create directory for that pid
        Directory* piddir = procfs_->tryAddShadowDirectory(fmt::format("{}", pid));
        verify(!!piddir, fmt::format("Unable to create /proc/{}", pid));

        // create symlink to that directory
        Path self("proc", "self");
        Path selfLink("proc", fmt::format("{}", pid));
        auto selfsymlink = ShadowSymlink::tryCreate(self, selfLink.absolute());
        verify(!!selfsymlink, "Unable to create /proc/self");
        Directory* selfParent = ensurePathExceptLast(self);
        selfParent->addFile(std::move(selfsymlink));

        // add symlink /proc/{pid}/exe to the executable path
        Path exe("proc", fmt::format("{}", pid), "exe");
        auto exesymlink = ShadowSymlink::tryCreate(exe, programFilePath.absolute());
        verify(!!exesymlink, "Unable to create /proc/{pid}/exe");
        Directory* exeParent = ensurePathExceptLast(exe);
        exeParent->addFile(std::move(exesymlink));
    }

    FS::~FS() = default;

    BitFlags<AccessMode> FS::toAccessMode(int flags) {
        BitFlags<AccessMode> am;
        if(Host::Open::isReadWrite(flags) || Host::Open::isReadOnly(flags)) am.add(AccessMode::READ);
        if(Host::Open::isReadWrite(flags) || Host::Open::isWriteOnly(flags)) am.add(AccessMode::WRITE);
        return am;
    }

    BitFlags<CreationFlags> FS::toCreationFlags(int flags) {
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

    BitFlags<StatusFlags> FS::toStatusFlags(int flags) {
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

    Permissions FS::fromMode(unsigned int mode) {
        return Permissions {
            Host::Mode::isUserReadable(mode),
            Host::Mode::isUserWritable(mode),
            Host::Mode::isUserExecutable(mode),
        };
    }

    FileDescriptor FS::insertNode(std::unique_ptr<File> file, BitFlags<AccessMode> accessMode, BitFlags<StatusFlags> statusFlags, bool closeOnExec) {
        File* filePtr = file.get();
        orphanFiles_.push_back(std::move(file));
        auto ofd = std::make_unique<OpenFileDescription>(filePtr, accessMode, statusFlags);
        FileDescriptor fd{std::move(ofd), closeOnExec};
        filePtr->ref();
        return fd;
    }

    std::optional<Path> FS::resolvePath(const Directory* cwd, const std::string& pathname) const {
        if(pathname.empty()) return {};
        if(pathname[0] == '/') {
            auto path = Path::tryCreate(pathname);
            if(!path) return {};
            return *path;
        } else {
            verify(!!cwd, "Null cwd directory");
            auto path = Path::tryCreate(pathname, cwd->path().absolute());
            if(!path) return {};
            return *path;
        }
    }

    std::optional<Path> FS::resolvePath(CurrentDirectoryOrDirectoryDescriptor dirfd, const std::string& pathname) const {
        if(!pathname.empty() && pathname[0] == '/') {
            auto path = Path::tryCreate(pathname);
            if(!path) return {};
            return *path;
        } else if(dirfd.isCurrentDirectory) {
            return resolvePath(dirfd.cwd, pathname);
        } else {
            OpenFileDescription* openFileDescription = dirfd.directoryDescriptor.openFiledescription.get();
            verify(!!openFileDescription, "Trying to resolve path for unopened directory");
            verify(openFileDescription->file()->isDirectory(), "Trying to resolve path for non-directory");
            const Directory* dir = static_cast<const Directory*>(openFileDescription->file());
            return resolvePath(dir, pathname);
        }
    }

    std::optional<Path> FS::resolvePath(CurrentDirectoryOrDirectoryDescriptor dirfd, const std::string& pathname, AllowEmptyPathname tag) const {
        if(pathname.empty()) {
            if(tag == AllowEmptyPathname::YES) {
                OpenFileDescription* openFileDescription = dirfd.directoryDescriptor.openFiledescription.get();
                verify(!!openFileDescription, "Trying to resolve path for unopened directory");
                return openFileDescription->file()->path();
            } else {
                return {};
            }
        } else if(pathname[0] == '/') {
            auto path = Path::tryCreate(pathname);
            if(!path) return {};
            return *path;
        } else if(dirfd.isCurrentDirectory) {
            return resolvePath(dirfd.cwd, pathname);
        } else {
            OpenFileDescription* openFileDescription = dirfd.directoryDescriptor.openFiledescription.get();
            verify(!!openFileDescription, "Trying to resolve path for unopened directory");
            verify(openFileDescription->file()->isDirectory(), "Trying to resolve path for non-directory");
            const Directory* dir = static_cast<const Directory*>(openFileDescription->file());
            return resolvePath(dir, pathname);
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

    ErrnoOr<FileDescriptor> FS::open(const Path& path,
            BitFlags<AccessMode> accessMode,
            BitFlags<CreationFlags> creationFlags,
            BitFlags<StatusFlags> statusFlags,
            Permissions permissions) {
        (void)permissions;
        // For some reason, 64-bit linux adds this flag without notification
        statusFlags.add(StatusFlags::LARGEFILE);

        bool canUseHostFile = true;
        if(accessMode.test(AccessMode::WRITE))       canUseHostFile = false;
        if(creationFlags.test(CreationFlags::CREAT)) canUseHostFile = false;
        if(creationFlags.test(CreationFlags::TRUNC)) canUseHostFile = false;
        if(statusFlags.test(StatusFlags::APPEND))    canUseHostFile = false;

        // Look if the file is already present in FS, open or closed.
        bool closeOnExec = creationFlags.test(CreationFlags::CLOEXEC);

        auto openNode = [&](File* filePtr) -> FileDescriptor {
            auto ofd = std::make_unique<OpenFileDescription>(filePtr, accessMode, statusFlags);
            auto descriptor = FileDescriptor{std::move(ofd), closeOnExec};
            filePtr->ref();
            return descriptor;
        };
        
        File* file = tryGetFile(path, FollowSymlink::YES);
        if(!!file) {
            auto descriptor = openNode(file);
            file->open();
            return ErrnoOr<FileDescriptor>(descriptor);
        }

        // Otherwise, try to create it
        if(canUseHostFile) {
            if(creationFlags.test(CreationFlags::DIRECTORY)) {
                // open the directory
                auto hostBackedDirectory = HostDirectory::tryCreate(path);
                if(!!hostBackedDirectory) {
                    // create and add the node to the filesystem
                    Directory* parent = ensurePathExceptLast(path);
                    HostDirectory* dir = parent->addFile<HostDirectory>(std::move(hostBackedDirectory));
                    dir->open();
                    return ErrnoOr<FileDescriptor>(openNode(dir));
                }
                
                // TODO: return the actual value of errno
                return ErrnoOr<FileDescriptor>(-ENOENT);
            } else {
                // try open the directory
                auto hostBackedDirectory = HostDirectory::tryCreate(path);
                if(!!hostBackedDirectory) {
                    // create and add the node to the filesystem
                    Directory* parent = ensurePathExceptLast(path);
                    HostDirectory* dir = parent->addFile<HostDirectory>(std::move(hostBackedDirectory));
                    dir->open();
                    return ErrnoOr<FileDescriptor>(openNode(dir));
                }

                // try open the file
                auto hostBackedFile = HostFile::tryCreate(path, accessMode, closeOnExec);
                if(!!hostBackedFile) {
                    Directory* parent = ensurePathExceptLast(path);
                    HostFile* file = parent->addFile<HostFile>(std::move(hostBackedFile));
                    // create and add the node to the filesystem
                    file->open();
                    return ErrnoOr<FileDescriptor>(openNode(file));
                }

                // try open device
                auto hostDevice = HostDevice::tryCreate(path);
                if(!!hostDevice) {
                    // create and add the node to the filesystem
                    Directory* parent = ensurePathExceptLast(path);
                    HostDevice* device = parent->addFile<HostDevice>(std::move(hostDevice));
                    device->open();
                    return ErrnoOr<FileDescriptor>(openNode(device));
                }

                // TODO: return the actual value of errno
                return ErrnoOr<FileDescriptor>(-ENOENT);
            }
        } else {
            // open the file
            bool createIfNotFound = creationFlags.test(CreationFlags::CREAT);
            auto shadowFile = ShadowFile::tryCreate(path, createIfNotFound);
            if(!!shadowFile) {
                if(creationFlags.test(CreationFlags::TRUNC)) shadowFile->truncate(0);
                shadowFile->setWritable(accessMode.test(AccessMode::WRITE));
                
                // create and add the node to the filesystem
                Directory* parent = ensurePathExceptLast(path);
                ShadowFile* file = parent->addFile<ShadowFile>(std::move(shadowFile));
                file->open();
                return ErrnoOr<FileDescriptor>(openNode(file));
            }

            // try open device
            auto shadowDevice = ShadowDevice::tryCreate(path, closeOnExec);
            if(!!shadowDevice) {
                // create and add the node to the filesystem
                Directory* parent = ensurePathExceptLast(path);
                Device* device = parent->addFile(std::move(shadowDevice));
                device->open();
                return ErrnoOr<FileDescriptor>(openNode(device));
            }

            // TODO: return the actual value of errno
            return ErrnoOr<FileDescriptor>(-ENOENT);
        }
        return ErrnoOr<FileDescriptor>(-EINVAL);
    }

    int FS::mkdir(const Path& path) {
        ensureCompletePath(path);
        return 0;
    }

    int FS::rename(const Path& oldpath, const Path& newpath) {
        auto file = tryTakeFile(oldpath);
        if(!file) return -ENOENT;
        if(!file->isShadow()) warn("renaming non-shadow files is broken");
        auto* newdir = ensurePathExceptLast(newpath);
        verify(!!newdir, "Unable to create new directory");
        file->rename(newdir, newpath.last());
        newdir->addFile(std::move(file));
        return 0;
    }
    
    int FS::unlink(const Path& path) {
        File* filePtr = tryGetFile(path, FollowSymlink::YES);
        if(!filePtr) return -ENOENT;
        if(filePtr->refCount() > 0) {
            filePtr->setDeleteAfterClose();
        } else {
            auto file = tryTakeFile(path);
            // let the file die here
        }
        return 0;
    }

    ErrnoOrBuffer FS::readlink(const Path& path, size_t bufferSize) {
        File* file = tryGetFile(path, FollowSymlink::NO);
        if(!!file) {
            if(!file->isSymlink()) return ErrnoOrBuffer(-EINVAL);
            Symlink* symlink = static_cast<Symlink*>(file);
            return symlink->readlink(bufferSize);
        } else {
            // create and add the node to the filesystem
            auto hostSymlink = HostSymlink::tryCreate(path);
            if(!hostSymlink) return ErrnoOrBuffer(-EINVAL);
            Directory* parent = ensurePathExceptLast(path);
            Symlink* symlink = parent->addFile(std::move(hostSymlink));
            verify(symlink->isSymlink());
            return symlink->readlink(bufferSize);
        }
    }

    int FS::access(const Path& path, int mode) const {
        return Host::access(path.absolute(), mode);
    }

    ErrnoOr<FileDescriptor> FS::memfd_create(const std::string& name, unsigned int flags) {
        verify(!Host::MemfdFlags::isOther(flags), "Allow (and ignore) cloexec and allow_sealing");
        auto shadowFile = ShadowFile::tryCreate(name);
        if(!shadowFile) return ErrnoOr<FileDescriptor>{-ENOMEM};
        shadowFile->setDeleteAfterClose();
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        return ErrnoOr<FileDescriptor>(insertNode(std::move(shadowFile), accessMode, statusFlags, Host::MemfdFlags::isCloseOnExec(flags)));
    }

    BlockOr<ErrnoOrBuffer> FS::read(FileDescriptor fd, size_t count) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer{-EBADF};
        File* file = openFileDescription->file();
        if(!file->isReadable()) ErrnoOrBuffer{-EBADF};
        return openFileDescription->read(count);
    }

    ErrnoOrBuffer FS::pread(FileDescriptor fd, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer{-EBADF};
        File* file = openFileDescription->file();
        if(!file->isReadable()) ErrnoOrBuffer{-EBADF};
        return openFileDescription->pread(count, offset);
    }

    ssize_t FS::readv(FileDescriptor fd, std::vector<Buffer>* buffers) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isReadable()) return -EBADF;
        ssize_t nbytes = 0;
        for(Buffer& buf : *buffers) {
            auto readResult = openFileDescription->read(buf.size());
            verify(!readResult.isBlocking(), "blocking read not handled in readv");
            ssize_t ret = readResult.value().errorOrWith<ssize_t>([&](const Buffer& readBuffer) {
                verify(readBuffer.size() <= buf.size());
                ::memcpy(buf.data(), readBuffer.data(), readBuffer.size());
                return (ssize_t)readBuffer.size();
            });
            if(ret < 0) return ret;
            nbytes += ret;
        }
        return nbytes;
    }

    ssize_t FS::write(FileDescriptor fd, const u8* buf, size_t count) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        if(!file->isWritable()) return -EBADF;
        return openFileDescription->write(buf, count);
    }

    ssize_t FS::pwrite(FileDescriptor fd, const u8* buf, size_t count, off_t offset) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        if(!file->isWritable()) return -EBADF;
        return openFileDescription->pwrite(buf, count, offset);
    }

    ssize_t FS::writev(FileDescriptor fd, const std::vector<Buffer>& buffers) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
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

    ErrnoOrBuffer FS::stat(const Path& path) {
        File* file = tryGetFile(path, FollowSymlink::YES);
        if(!!file) return file->stat();
        return Host::stat(path.absolute());
    }

    ErrnoOrBuffer FS::fstat(FileDescriptor fd) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->stat();
    }

    ErrnoOrBuffer FS::statx(const Path& path, int flags, unsigned int mask) { // NOLINT(readability-convert-member-functions-to-static)
        FollowSymlink followSymlink = Host::Fstatat::isSymlinkNofollow(flags) ? FollowSymlink::NO : FollowSymlink::YES;
        File* file = tryGetFile(path, followSymlink);
        if(!!file) {
            return file->statx(mask);
        } else {
            // if we don't know the file, delegating to the host is probably fine
            return Host::statx(Host::cwdfd(), path.absolute(), flags, mask);
        }
    }

    ErrnoOrBuffer FS::fstatat64(const Path& path, int flags) {
        verify(!Host::Fstatat::isNoAutomount(flags), "no automount not supported");
        FollowSymlink followSymlink = Host::Fstatat::isSymlinkNofollow(flags) ? FollowSymlink::YES : FollowSymlink::NO;
        File* file = tryGetFile(path, followSymlink);
        if(!file) {
            return ErrnoOrBuffer(-ENOENT);
        } else {
            return file->stat();
        }
    }

    ErrnoOrBuffer FS::fstatfs(FileDescriptor fd) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->file()->statfs();
    }

    off_t FS::lseek(FileDescriptor fd, off_t offset, int whence) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        return openFileDescription->lseek(offset, whence);
    }

    void FS::checkFileRefCount(File* file) const {
        (void)file;
        // assert(file->refCount() == std::count_if(fileDescriptors_.begin(), fileDescriptors_.end(), [&](auto& desc) {
        //     if(!desc) return false;
        //     return desc->openFiledescription->file() == file;
        // }));
    }

    int FS::close(FileDescriptor fd) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        checkFileRefCount(file);
        file->unref();
#ifndef NDEBUG
        checkFileRefCount(file);
#endif
        if(file->refCount() == 0) {
            file->close();
            if(file->isPipe()) {
                removeClosedPipes();
            }
            if(!file->keepAfterClose() || file->deleteAfterClose()) {
                auto path = file->path();
                unlink(path);
                removeFromOrphans(file);
            }
        }
        return 0;
    }

    void FS::removeFromOrphans(File* file) {
        auto it = std::remove_if(orphanFiles_.begin(), orphanFiles_.end(),
                [=](const auto& f) { return f.get() == file; });
        orphanFiles_.erase(it, orphanFiles_.end());
    }

    void FS::removeClosedPipes() {
        pipes_.erase(std::remove_if(pipes_.begin(), pipes_.end(),
            [](const auto& pipe) { return pipe->isClosed(); }), pipes_.end());
    }

    ErrnoOrBuffer FS::getdents64(FileDescriptor fd, size_t count) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        File* file = openFileDescription->file();
        return file->getdents64(count);
    }

    int FS::fcntl(FileDescriptor& descriptor, int cmd, int arg) {
        OpenFileDescription* openFileDescription = descriptor.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        std::optional<int> emulatedRet;
        // If we can do it in FS alone, do it here
        // and check if we need to do it on the file as well
        bool callFcntlOnFile = true;
        switch(Host::Fcntl::toCommand(cmd)) {
            case Host::Fcntl::DUPFD: {
                verify(false, "cannot do dupfd in FS::fcntl");
                break;
            }
            case Host::Fcntl::DUPFD_CLOEXEC: {
                verify(false, "cannot do dupfd_cloexec in FS::fcntl");
                break;
            }
            case Host::Fcntl::GETFD: {
                emulatedRet = descriptor.closeOnExec ? Host::Fcntl::fdCloExec() : 0;
                break;
            }
            case Host::Fcntl::SETFD: {
                bool currentFlag = descriptor.closeOnExec;
                if(Host::Open::isCloseOnExec(arg) && !currentFlag) descriptor.closeOnExec = true;
                if(!Host::Open::isCloseOnExec(arg) && currentFlag) descriptor.closeOnExec = false;
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
                // File access mode and file creation flags in arg are ignored
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

    ErrnoOrBuffer FS::ioctl(FileDescriptor fd, Ioctl request, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        return openFileDescription->ioctl(request, buffer);
    }

    int FS::flock(FileDescriptor fd, int operation) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
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

    int FS::fallocate(FileDescriptor fd, int mode, off_t offset, off_t len) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        verify(file->isRegularFile(), "fallocate not implemented on non-regular files");
        verify(file->isShadow(), "fallocate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        return shadowFile->fallocate(mode, offset, len);
    }

    int FS::truncate(const Path& path, off_t length) {
        File* file = tryGetFile(path, FollowSymlink::YES);
        if(!file) return -ENOENT;
        verify(file->isRegularFile(), "truncate not implemented on non-regular files");
        verify(file->isShadow(), "truncate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        shadowFile->truncate((size_t)length);
        return 0;
    }

    int FS::ftruncate(FileDescriptor fd, off_t length) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        File* file = openFileDescription->file();
        verify(file->isRegularFile(), "fallocate not implemented on non-regular files");
        verify(file->isShadow(), "fallocate not implemented on non-shadow files");
        ShadowFile* shadowFile = static_cast<ShadowFile*>(file);
        shadowFile->truncate((size_t)length);
        return 0;
    }

    ErrnoOr<FileDescriptor> FS::eventfd2(unsigned int initval, int flags) {
        verify(!Host::Eventfd2Flags::isSemaphore(flags), "only closeOnExec and nonBlock are allowed on eventfd2");
        std::unique_ptr<Event> event = Event::tryCreate(initval, flags);
        verify(!!event, "Unable to create event");
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        if(Host::Eventfd2Flags::isNonBlock(flags)) statusFlags.add(StatusFlags::NONBLOCK);
        bool closeOnExec = Host::Eventfd2Flags::isCloseOnExec(flags);
        auto fd = insertNode(std::move(event), accessMode, statusFlags, closeOnExec);
        return ErrnoOr<FileDescriptor>(fd);
    }

    ErrnoOr<FileDescriptor> FS::epoll_create1(int flags) {
        std::unique_ptr<Epoll> epoll = std::make_unique<Epoll>(flags);
        verify(!!epoll, "Unable to create epoll");
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        bool closeOnExec = Host::EpollFlags::isCloseOnExec(flags);
        auto descriptor = insertNode(std::move(epoll), accessMode, statusFlags, closeOnExec);
        return ErrnoOr<FileDescriptor>(descriptor);
    }

    int FS::epoll_ctl(FileDescriptor epfd, int op, FileDescriptor fd, BitFlags<EpollEventType> events, u64 data) {
        OpenFileDescription* openFileDescription = epfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isEpoll()) return -EBADF;
        Epoll* epoll = static_cast<Epoll*>(openFileDescription->file());
        if(Host::EpollCtlOp::isAdd(op)) {
            events.add(EpollEventType::HANGUP);
            auto ret = epoll->addEntry(fd.openFiledescription.get(), events.toUnderlying(), data);
            return ret.errorOr(0);
        } else if(Host::EpollCtlOp::isMod(op)) {
            auto ret = epoll->changeEntry(fd.openFiledescription.get(), events.toUnderlying(), data);
            return ret.errorOr(0);
        } else {
            verify(Host::EpollCtlOp::isDel(op), "Unknown epoll_ctl op");
            auto ret = epoll->deleteEntry(fd.openFiledescription.get());
            return ret.errorOr(0);
        }
    }

    int FS::epollWaitImmediate(FileDescriptor epfd, std::vector<EpollEvent>* events) {
        OpenFileDescription* openFileDescription = epfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isEpoll()) return -EBADF;
        doEpollWait(epfd, events);
        return (int)events->size();
    }

    void FS::doEpollWait(FileDescriptor epfd, std::vector<EpollEvent>* events) {
        verify(!!events);
        OpenFileDescription* openFileDescription = epfd.openFiledescription.get();
        verify(!!openFileDescription, "No open file description for epoll wait");
        verify(openFileDescription->file()->isEpoll(), "Cannot perform epoll wait on non-epoll instance");
        Epoll* instance = static_cast<Epoll*>(openFileDescription->file());
        instance->forEachEntryInInterestList([&](void* ofd, u32 ev, u64 data) {
            OpenFileDescription* openFileDescription = (OpenFileDescription*)ofd;
            if(!openFileDescription) {
                events->push_back(EpollEvent{
                    BitFlags<EpollEventType>{EpollEventType::HANGUP},
                    0,
                });
                return;
            }
            File* file = openFileDescription->file();
            verify(file->isPollable(), [&]() { fmt::print("ofd={:p} is not pollable\n", ofd); });

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

            if(!outputEvents.any()) return;
            events->push_back(EpollEvent{
                outputEvents,
                data,
            });
        });
    }

    ErrnoOr<FileDescriptor> FS::socket(int domain, int type, int protocol) {
        auto socket = Socket::tryCreate(domain, type, protocol);
        // TODO: return the actual value of errno
        if(!socket) return ErrnoOr<FileDescriptor>{-EINVAL};
        BitFlags<AccessMode> accessMode { AccessMode::READ, AccessMode::WRITE };
        BitFlags<StatusFlags> statusFlags { };
        if(Host::SocketType::isNonBlock(type)) statusFlags.add(StatusFlags::NONBLOCK);
        bool closeOnExec = Host::SocketType::isCloseOnExec(type);
        auto descriptor = insertNode(std::move(socket), accessMode, statusFlags, closeOnExec);
        return ErrnoOr<FileDescriptor>(descriptor);
    }

    int FS::connect(FileDescriptor sockfd, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->connect(buffer);
    }

    int FS::bind(FileDescriptor sockfd, const Buffer& name) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->bind(name);
    }

    int FS::shutdown(FileDescriptor sockfd, int how) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->shutdown(how);
    }

    ErrnoOrBuffer FS::getpeername(FileDescriptor sockfd, u32 buffersize) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getpeername(buffersize);
    }

    ErrnoOrBuffer FS::getsockname(FileDescriptor sockfd, u32 buffersize) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getsockname(buffersize);
    }

    ErrnoOrBuffer FS::getsockopt(FileDescriptor sockfd, int level, int optname, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOrBuffer(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOrBuffer(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->getsockopt(level, optname, buffer);
    }

    int FS::setsockopt(FileDescriptor sockfd, int level, int optname, const Buffer& buffer) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -EBADF;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->setsockopt(level, optname, buffer);
    }

    ErrnoOr<BufferAndReturnValue<int>> FS::pollImmediate(const std::vector<PollData>& pfds) {
        std::vector<PollFd> rfds;
        rfds.resize(pfds.size());
        for(size_t i = 0; i < pfds.size(); ++i) {
            auto& rfd = rfds[i];
            rfd.fd = pfds[i].fd;
            rfd.events = pfds[i].events;
            rfd.revents = pfds[i].revents;
            // check that all fds are pollable and have a host-side fd
            FileDescriptor descriptor = pfds[i].descriptor;
            if(!descriptor.openFiledescription) {
                rfd.revents = pfds[i].revents | PollEvent::INVALID_REQUEST;
                continue;
            }
            File* file = descriptor.openFiledescription->file();
            verify(file->isPollable(), [&]() { fmt::print("file is not pollable\n"); });

            bool testRead = ((rfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ);
            bool testWrite = ((rfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE);
            if(testRead && file->canRead())   rfd.revents = rfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) rfd.revents = rfd.revents | PollEvent::CAN_WRITE;
        }
        int ret = 0;
        Buffer buffer(rfds.size()*sizeof(PollFd), 0);
        ::memcpy(buffer.data(), rfds.data(), buffer.size());
        BufferAndReturnValue<int> bufferAndRetVal {
            std::move(buffer),
            ret,
        };
        return ErrnoOr<BufferAndReturnValue<int>>(std::move(bufferAndRetVal));
    }

    void FS::doPoll(std::vector<PollData>* data) {
        if(!data) return;
        for(PollData& pollfd : *data) {
            FileDescriptor descriptor = pollfd.descriptor;
            verify(!!descriptor.openFiledescription, [&]() {
                fmt::print("Polling on non-existing open file description\n");
            });
            File* file = descriptor.openFiledescription->file();
            verify(file->isPollable(), [&]() { fmt::print("Attempting to poll non-pollable file\n"); });
            bool testRead = (pollfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ;
            bool testWrite = (pollfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE;
            pollfd.revents = PollEvent::NONE;
            if(testRead && file->canRead())   pollfd.revents = pollfd.revents | PollEvent::CAN_READ;
            if(testWrite && file->canWrite()) pollfd.revents = pollfd.revents | PollEvent::CAN_WRITE;
        }
    }

    int FS::selectImmediate(SelectData* selectData) {
        if(!selectData) return 0;
        for(size_t i = 0; i < selectData->fds.size(); ++i) {
            bool testRead = selectData->readfds.test(i);
            bool testWrite = selectData->writefds.test(i);
            if(!testRead && !testWrite) continue;

            selectData->readfds.reset(i);
            selectData->writefds.reset(i);

            // check that all fds are pollable and have a host-side fd
            OpenFileDescription* openFileDescription = selectData->fds[i].openFiledescription.get();
            if(!openFileDescription) return -EBADF;

            File* file = openFileDescription->file();
            verify(file->isPollable(), [&]() { fmt::print("file is not pollable\n"); });

            if(testRead && file->canRead())   selectData->readfds.set(i);
            if(testWrite && file->canWrite()) selectData->writefds.set(i);
        }
        return 0;
    }

    ErrnoOr<std::pair<FileDescriptor, FileDescriptor>> FS::pipe2(int flags) {
        using ReturnType = ErrnoOr<std::pair<FileDescriptor, FileDescriptor>>;
        auto pipe = Pipe::tryCreate(flags);
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
        FileDescriptor reader = insertNode(std::move(readingSide), BitFlags<AccessMode>{AccessMode::READ}, statusFlags, closeOnExec);
        FileDescriptor writer = insertNode(std::move(writingSide), BitFlags<AccessMode>{AccessMode::WRITE}, statusFlags, closeOnExec);
        return ReturnType(std::make_pair(reader, writer));
    }

    ErrnoOr<std::pair<Buffer, Buffer>> FS::recvfrom(FileDescriptor sockfd, size_t len, int flags, bool requireSrcAddress) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        if(!openFileDescription->file()->isSocket()) return ErrnoOr<std::pair<Buffer, Buffer>>(-EBADF);
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->recvfrom(len, flags, requireSrcAddress);

    }
    ssize_t FS::recvmsg(FileDescriptor sockfd, int flags, Message* message) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
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

    ssize_t FS::send(FileDescriptor sockfd, const Buffer& buffer, int flags) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
        if(!openFileDescription) return -EBADF;
        if(!openFileDescription->file()->isSocket()) return -ENOTSOCK;
        Socket* socket = static_cast<Socket*>(openFileDescription->file());
        return socket->send(buffer, flags);
    }

    ssize_t FS::sendmsg(FileDescriptor sockfd, int flags, const Message& message) {
        OpenFileDescription* openFileDescription = sockfd.openFiledescription.get();
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

    std::string FS::filename(FileDescriptor fd) {
        OpenFileDescription* openFileDescription = fd.openFiledescription.get();
        if(!openFileDescription) return "Unknown";
        File* file = openFileDescription->file();
        return file->path().absolute();
    }

    void FS::dumpSummary() const {
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