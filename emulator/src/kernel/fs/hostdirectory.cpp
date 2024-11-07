#include "kernel/fs/hostdirectory.h"
#include "kernel/fs/fs.h"
#include "kernel/fs/path.h"
#include "kernel/kernel.h"
#include "kernel/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace kernel {

    std::unique_ptr<HostDirectory> HostDirectory::tryCreateRoot(FS* fs) {
        return std::unique_ptr<HostDirectory>(new HostDirectory(fs, nullptr, ""));
    }

    std::unique_ptr<HostDirectory> HostDirectory::tryCreate(FS* fs, Directory* parent, std::string name) {
        std::string pathname;
        if(!parent) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        int flags = O_RDONLY | O_CLOEXEC;
        int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
        if(fd < 0) return {};

        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        // check that the file is a directory
        struct stat s;
        if(::fstat(fd, &s) < 0) {
            return {};
        }
        
        mode_t fileType = (s.st_mode & S_IFMT);
        if (fileType != S_IFDIR) {
            // not a directory
            return {};
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        return std::unique_ptr<HostDirectory>(new HostDirectory(fs, containingDirectory, path->last()));
    }

    void HostDirectory::open() {
        if(!hostFd_) {
            std::string pathname = path();
            int flags = O_RDONLY | O_CLOEXEC;
            int fd = ::openat(AT_FDCWD, pathname.c_str(), flags);
            verify(fd >= 0, "Unable to open directory");
            hostFd_ = fd;
        }
    }

    void HostDirectory::close() {
        verify(!!hostFd_, "Trying to close un-opened directory");
        int ret = ::close(hostFd_.value());
        verify(ret == 0, "Closing directory failed");
        hostFd_.reset();
    }

    ErrnoOrBuffer HostDirectory::getdents64(size_t count) {
        verify(!!hostFd_, "Directory must be opened first");
        return fs_->kernel().host().getdents64(Host::FD{hostFd_.value()}, count);
    }

}