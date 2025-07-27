#include "kernel/linux/fs/procfs.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"

namespace kernel {

    ProcFS* ProcFS::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        auto procfs = std::unique_ptr<ProcFS>(new ProcFS(fs, containingDirectory, path->last()));
        return containingDirectory->addFile(std::move(procfs));
    }

    ErrnoOrBuffer ProcFS::stat() {
        verify(false, "implement stat on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ProcFS::statfs() {
        verify(false, "implement statfs on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ProcFS::getdents64(size_t) {
        verify(false, "implement getdents on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

}