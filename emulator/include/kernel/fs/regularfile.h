#ifndef REGULARFILE_H
#define REGULARFILE_H

#include "kernel/fs/directory.h"
#include "kernel/fs/file.h"
#include "utils.h"
#include <cassert>

namespace kernel {

    class RegularFile : public File {
    public:
        explicit RegularFile(FS* fs, Directory* dir, std::string name) : File(fs, dir, std::move(name)) { }

        bool isRegularFile() const override final { return true; }
    };

}

#endif