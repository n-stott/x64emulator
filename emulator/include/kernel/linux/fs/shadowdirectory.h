#ifndef SHADOWDIRECTORY_H
#define SHADOWDIRECTORY_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/file.h"
#include <memory>
#include <string>

namespace kernel::gnulinux {

    class Directory;
    class FS;
    class Path;

    class ShadowDirectory : public Directory {
    public:
        static std::unique_ptr<ShadowDirectory> tryCreate(const Path& path);
        
        bool isShadow() const override { return true; }

        ErrnoOrBuffer getdents64(size_t count) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;
        ErrnoOrBuffer statx(unsigned int mask) override;

        std::string className() const override { return "ShadowDirectory"; }

    private:
        ShadowDirectory(std::string name) : Directory(std::move(name)) { }

    };

}

#endif