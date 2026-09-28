#include "kernel/linux/fs/hostdirectory.h"
#include "kernel/linux/fs/openfiledescription.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace kernel::gnulinux {

    std::shared_ptr<Host::FileHandle> HostDirectory::tryGetHandle() const {
        auto absolutePath = path().absolute();
        auto handle = Host::tryOpen(absolutePath, Host::FileType::DIRECTORY, Host::Purgeable::NO, Host::CloseOnExec::YES);
        return handle;
    }

    std::unique_ptr<HostDirectory> HostDirectory::tryCreateRoot() {
        return std::unique_ptr<HostDirectory>(new HostDirectory(""));
    }

    std::unique_ptr<HostDirectory> HostDirectory::tryCreate(const Path& path) {
        std::string pathname = path.absolute();
        auto handle = Host::tryOpen(pathname,
                Host::FileType::DIRECTORY,
                Host::Purgeable::NO,
                Host::CloseOnExec::YES);
        if(!handle) return {};
        return std::unique_ptr<HostDirectory>(new HostDirectory(path.last()));
    }

    void HostDirectory::open() {

    }

    void HostDirectory::close() {
        auto absolutePath = path().absolute();
        Host::purgeHandle(absolutePath.c_str());
    }

    off_t HostDirectory::lseek(OpenFileDescription& ofd, off_t offset, int whence) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        off_t ret = [&]() {
            if(whence == SEEK_CUR) {
                return handle->lseek(ofd.offset() + offset, Host::FileHandle::SEEK::SET);
            } else if(whence == SEEK_SET) {
                return handle->lseek(offset, Host::FileHandle::SEEK::SET);
            } else {
                verify(whence == SEEK_END);
                return handle->lseek(offset, Host::FileHandle::SEEK::END);
            }
        }();
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer HostDirectory::stat() {
        std::string path = this->path().absolute();
        return Host::stat(path);
    }

    ErrnoOrBuffer HostDirectory::statfs() {
        std::string path = this->path().absolute();
        return Host::statfs(path);
    }

    ErrnoOrBuffer HostDirectory::statx(unsigned int mask) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return handle->statx(mask); // NOLINT(bugprone-unchecked-optional-access)
    }

    ErrnoOrBuffer HostDirectory::getdents64(size_t count) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        return handle->getdents64(count); // NOLINT(bugprone-unchecked-optional-access)
    }

    std::optional<int> HostDirectory::fcntl(FcntlCommand cmd, int arg) {
        auto handle = tryGetHandle();
        verify(!!handle, "Unable to obtain handle");
        int hostcmd = Host::Fcntl::fromCommand(cmd);
        return Host::fcntl(handle->fd(), hostcmd, arg); // NOLINT(bugprone-unchecked-optional-access)
    }

}