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

    std::unique_ptr<HostDirectory> HostDirectory::tryCreateRoot() {
        return std::unique_ptr<HostDirectory>(new HostDirectory(""));
    }

    std::unique_ptr<HostDirectory> HostDirectory::tryCreate(const Path& path) {
        std::string pathname = path.absolute();
        auto handle = Host::tryOpen(pathname.c_str(),
                Host::FileType::DIRECTORY,
                Host::CloseOnExec::YES);
        if(!handle) return {};
        return std::unique_ptr<HostDirectory>(new HostDirectory(path.last()));
    }

    void HostDirectory::open() {
        if(!handle_) {
            std::string pathname = path().absolute();
            handle_ = Host::tryOpen(pathname.c_str(),
                    Host::FileType::DIRECTORY,
                    Host::CloseOnExec::YES);
            verify(handle_.has_value(), "Unable to open directory \"" + pathname + "\"");
        }
    }

    void HostDirectory::close() {
        verify(!!handle_, "Trying to close un-opened directory");
        handle_.reset();
    }

    off_t HostDirectory::lseek(OpenFileDescription& ofd, off_t offset, int whence) {
        verify(!!handle_, "Trying to close un-opened directory");
        off_t ret = [&]() {
            if(whence == SEEK_CUR) {
                return handle_->lseek(ofd.offset() + offset, Host::FileHandle::SEEK::SET);
            } else if(whence == SEEK_SET) {
                return handle_->lseek(offset, Host::FileHandle::SEEK::SET);
            } else {
                verify(whence == SEEK_END);
                return handle_->lseek(offset, Host::FileHandle::SEEK::END);
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
        if(!handle_) open();
        verify(!!handle_, "Directory must be opened first");
        return handle_->statx(mask); // NOLINT(bugprone-unchecked-optional-access)
    }

    ErrnoOrBuffer HostDirectory::getdents64(size_t count) {
        verify(!!handle_, "Directory must be opened first");
        return handle_->getdents64(count); // NOLINT(bugprone-unchecked-optional-access)
    }

    std::optional<int> HostDirectory::fcntl(int cmd, int arg) {
        verify(!!handle_, "Directory must be opened first");
        return Host::fcntl(handle_->fd(), cmd, arg); // NOLINT(bugprone-unchecked-optional-access)
    }

}