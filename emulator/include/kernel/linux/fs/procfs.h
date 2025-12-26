#ifndef PROCFS_H
#define PROCFS_H

#include "kernel/linux/fs/directory.h"

namespace kernel::gnulinux {

    class Path;

    class ProcFS : public Directory {
    public:
        static std::unique_ptr<ProcFS> tryCreate(const Path& path);

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;
        ErrnoOrBuffer getdents64(size_t count) override;

    private:
        ProcFS(std::string name) : Directory(std::move(name)) { }
    };

}

#endif