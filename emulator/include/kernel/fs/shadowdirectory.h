#ifndef SHADOWDIRECTORY_H
#define SHADOWDIRECTORY_H

#include "kernel/fs/directory.h"
#include "kernel/fs/file.h"
#include <memory>
#include <string>

namespace kernel {

    class Directory;
    class FS;

    class ShadowDirectory : public Directory {
    public:
        static std::unique_ptr<ShadowDirectory> tryCreate(FS* fs, Directory* parent, std::string pathname);
        
        bool isShadow() const override { return true; }

        ErrnoOrBuffer getdents64(size_t count) override;

    private:
        ShadowDirectory(FS* fs, Directory* parent, std::string name) : Directory(fs, parent, std::move(name)) { }

    };

}

#endif