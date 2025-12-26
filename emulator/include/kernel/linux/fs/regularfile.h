#ifndef REGULARFILE_H
#define REGULARFILE_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/file.h"
#include "utils.h"
#include <cassert>

namespace kernel::gnulinux {

    class RegularFile : public File {
    public:
        explicit RegularFile(std::string name) : File(std::move(name)) { }

        bool isRegularFile() const override final { return true; }
    };

}

#endif