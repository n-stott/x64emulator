#include "kernel/linux/fs/shadowdirectory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "verify.h"

namespace kernel::gnulinux {

    std::unique_ptr<ShadowDirectory> ShadowDirectory::tryCreate(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent) {
            pathname = name;
        } else {
            pathname = (parent->path().absolute() + "/" + name);
        }

        auto path = Path::tryCreate(pathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        return std::unique_ptr<ShadowDirectory>(new ShadowDirectory(fs, containingDirectory, path->last()));
    }

    ErrnoOrBuffer ShadowDirectory::getdents64(size_t) {
        verify(false, "implement getdents on shadow directory");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ShadowDirectory::stat() {
        std::string path = this->path().absolute();
        return Host::stat(path);
    }

    ErrnoOrBuffer ShadowDirectory::statfs() {
        std::string path = this->path().absolute();
        auto errnoOrBuffer = Host::statfs(path);
        verify(!errnoOrBuffer.isError(), "ShadowDirectory::statfs returned error");
        return errnoOrBuffer;
    }

    ErrnoOrBuffer ShadowDirectory::statx(unsigned int mask) {
        verify(false, fmt::format("File::statx(mask={:#x}) not implemented for file type {}\n", mask, className()));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}