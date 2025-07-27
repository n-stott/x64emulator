#ifndef PROCFS_H
#define PROCFS_H

#include "kernel/linux/fs/directory.h"

namespace kernel {

    class ProcFS : public Directory {
    public:
        static ProcFS* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name);

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer getdents64(size_t count) override;

    private:
        ProcFS(FS* fs, Directory* dir, std::string name) : Directory(fs, dir, std::move(name)) { }
    };

}

#endif