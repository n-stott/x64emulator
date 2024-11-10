#ifndef DEVICE_H
#define DEVICE_H

#include "kernel/fs/directory.h"
#include "kernel/fs/file.h"
#include "utils.h"
#include <cassert>
#include <sys/types.h>

namespace kernel {

    class Device : public File {
    public:
        explicit Device(FS* fs, Directory* dir, std::string name) : File(fs, dir, std::move(name)) { }

        bool isDevice() const override final { return true; }
    };

}

#endif