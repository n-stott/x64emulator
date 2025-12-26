#include "kernel/linux/fs/procfs.h"
#include "kernel/linux/fs/path.h"

namespace kernel::gnulinux {

    std::unique_ptr<ProcFS> ProcFS::tryCreate(const Path& path) {
        return std::unique_ptr<ProcFS>(new ProcFS(path.last()));
    }

    ErrnoOrBuffer ProcFS::stat() {
        verify(false, "implement stat on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ProcFS::statfs() {
        verify(false, "implement statfs on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ProcFS::statx(unsigned int mask) {
        verify(false, fmt::format("File::statx(mask={:#x}) not implemented for file type {}\n", mask, className()));
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ProcFS::getdents64(size_t) {
        verify(false, "implement getdents on procfs");
        return ErrnoOrBuffer(-ENOTSUP);
    }

}